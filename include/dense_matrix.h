// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <mkl.h>

#include "Eigen/Dense"

#include "types.h"
// types.h is included before influences the behavior of Eigen and, therefore,
// Spectra
#include "spectra-master/include/SymEigsSolver.h"

#include "hyperparams.h"
#include "mat_utils.h"
#include "parallel.h"
#include "timer.h"
#include "utils.h"

namespace ISLE {
  class WordCountSparseMatrix;
  template<class FPTYPE>
  class FloatingPointSparseMatrix;

  template<class T>
  class DenseMatrix {
   protected:
    word_id_t _vocab_size;  // Size of the vocabulary
    doc_id_t  _num_docs;    // Number of documents
    T *       A;            // Storage
    bool      _alloc;       // Allocate storage for A?

   public:
    DenseMatrix(const word_id_t d_, const doc_id_t s_,
                bool alloc = true);  // If false, do not allocate A.

    ~DenseMatrix();

    inline T *const data() const {
      return A;
    }

    // Input: word-id and doc-id, both in 0-based indexing
    // Output: A reference to the entry in the matrix
    inline T &elem_ref(word_id_t word, doc_id_t doc) {
      return data()[(size_t) word + ((size_t) doc) * (size_t) vocab_size()];
    }

    inline T elem(word_id_t word, doc_id_t doc) const {
      return data()[(size_t) word + ((size_t) doc) * (size_t) vocab_size()];
    }

    // Copy @n elements starting from (@offset_vocab,@offset_docs) to @dst
    void copy_col_to(T *const dst, const doc_id_t doc) const;

    void copy_col_from(const T *const src, const doc_id_t doc);

    inline doc_id_t num_docs() const {
      return _num_docs;
    }

    inline word_id_t vocab_size() const {
      return _vocab_size;
    }

    void find_top_words_above_threshold(
        const doc_id_t topic, const FPTYPE         threshold,
        std::vector<std::pair<word_id_t, FPTYPE>> &top_words) const;

    void print_words_above_threshold(
        const doc_id_t topic, const count_t threshold,
        const std::vector<std::string> &vocab_words);

    void find_n_top_words(
        const doc_id_t topic, const word_id_t      nwords,
        std::vector<std::pair<word_id_t, FPTYPE>> &top_words) const;

    void print_top_words(
        const std::vector<std::string> &vocab_words,
        const std::vector<std::pair<word_id_t, FPTYPE>> &top_words,
        std::ostream &stream = std::cout) const;

    void write_to_file(const std::string &filename) const;

    void write_to_file_as_sparse(const std::string &filename,
                                 const unsigned base = 1)  // 0-based or 1-based
        const;
  };

  template<class FPTYPE>
  class FloatingPointDenseMatrix;

  class WordCountDenseMatrix : public DenseMatrix<count_t> {
   public:
    WordCountDenseMatrix(word_id_t d_, doc_id_t s_);

    ~WordCountDenseMatrix();

    size_t populate(DocWordEntriesReader &data);
  };

  template<class FPTYPE>
  class FloatingPointDenseMatrix : public DenseMatrix<FPTYPE> {
    FPTYPE *svd_temp;
    FPTYPE *U;
    FPTYPE *VT;
    FPTYPE *Sigma;    // Pointers for full SVD calculation, A=U*Sigma*VT
    FPTYPE *SigmaVT;  // Sigma * VT obtained from truncatated Spectra solve

    Eigen::MatrixX BBT;
    Eigen::MatrixX U_Spectra;  // first few eig vectors of this*this^T

    MKL_INT num_singular_vals;  // #Singular values

    using DenseMatrix<FPTYPE>::A;
    using DenseMatrix<FPTYPE>::_alloc;
    using DenseMatrix<FPTYPE>::num_docs;
    using DenseMatrix<FPTYPE>::vocab_size;
    using DenseMatrix<FPTYPE>::data;
    using DenseMatrix<FPTYPE>::copy_col_to;

   public:
    FloatingPointDenseMatrix(word_id_t d, doc_id_t s);

    ~FloatingPointDenseMatrix();

    void populate_from_sparse(const FloatingPointSparseMatrix<FPTYPE> &from);

    FPTYPE frobenius() const;

    void initialize_for_full_svd();

    FPTYPE singular_val(int k) const;

    void cleanup_full_svd();

    void initialize_for_eigensolver(const doc_id_t num_topics);

    void compute_Spectra(const doc_id_t num_topics);

    void copy_spectraSigmaVT_from(
        FloatingPointDenseMatrix<FPTYPE> &from, const doc_id_t k,
        bool hardCopy = false);  // true for memcpy, false for alias

    void copy_spectraSigmaVT_from(FloatingPointSparseMatrix<FPTYPE> &from,
                                  const doc_id_t k, bool hardCopy = false);

    void left_multiply_by_U_Spectra(FPTYPE *const out, const FPTYPE *in,
                                    const doc_id_t ld_in, const doc_id_t ncols);

    FPTYPE *get_ptr_to_spectraSigmaVT();

    void compare_LAPACK_Spectra(const doc_id_t num_topics,
                                const double   U_TOLERANCE,
                                const double   VT_TOLERANCE);

    void cleanup_after_eigensolver();

    void populate_with_topk_singulars(const doc_id_t                    k,
                                      FloatingPointDenseMatrix<FPTYPE> &from);

    FPTYPE distsq_point_to_center(const doc_id_t d, const FPTYPE *const center);

    void distsq_alldocs_to_centers(
        const word_id_t dim, doc_id_t num_centers, const FPTYPE *const centers,
        const FPTYPE *const centers_l2sq, doc_id_t num_docs,
        const FPTYPE *const docs, const FPTYPE *const docs_l2sq,
        FPTYPE *dist_matrix,
        FPTYPE *ones_vec =
            NULL);  // Scratchspace of num_docs size and init to 1.0

    void distsq_to_closest_center(
        const word_id_t dim, doc_id_t num_centers, const FPTYPE *const centers,
        const FPTYPE *const centers_l2sq, doc_id_t num_docs,
        const FPTYPE *const docs, const FPTYPE *const docs_l2sq,
        FPTYPE *const min_dist,
        FPTYPE *      ones_vec =
            NULL);  // Scratchspace of num_docs size and init to 1.0

    void update_min_distsq_to_centers(
        const word_id_t dim, const doc_id_t num_centers,
        const FPTYPE *const centers, doc_id_t num_docs,
        const FPTYPE *const docs, const FPTYPE *const docs_l2sq,
        FPTYPE *const min_dist,
        FPTYPE *      dist = NULL,  // preallocated scratch of space num_docs
        FPTYPE *      ones_vec =
            NULL,  // Scratchspace of num_docs size and init to 1.
        const bool                 weighted = false,
        const std::vector<size_t> &weights = std::vector<size_t>());

    FPTYPE kmeanspp(const doc_id_t k, std::vector<doc_id_t> &centers,
                    const bool                 weighted = false,
                    const std::vector<size_t> &weights = std::vector<size_t>());

    FPTYPE kmeansbb(const doc_id_t k, FPTYPE *final_centers_coords);

    FPTYPE kmeansmcmc(const doc_id_t k, std::vector<doc_id_t> &centers);

    FPTYPE kmeans_init(const int num_centers, const int max_reps,
                       const int              method,
                       std::vector<doc_id_t> &best_seed,  // Wont be initialized
                                                          // if method==KMEANSBB
                       FPTYPE *const best_centers_coords =
                           NULL);  // Wont be initialized if null

#define EIGEN_SOURCE_MKL 1
#define EIGEN_SOURCE_SPECTRA 2
    // Input @k: number of centers, @centers: reference to vector of indices of
    // chosen seeds
    // Input @spectrum_source: MKL or Spectra
    // Output: Sum of distances of all points to chosen seeds
    // All calulations are done on the column space of Sigma*VT of the truncated
    // SVD
    FPTYPE kmeanspp_on_col_space(doc_id_t k, std::vector<doc_id_t> &centers,
                                 int spectrum_source);

    void closest_centers(const doc_id_t      num_centers,
                         const FPTYPE *const centers,
                         const FPTYPE *const docs_l2sq, doc_id_t *center_index,
                         FPTYPE *const dist_matrix);  // Scratch init to
                                                      // num_centers*num_docs()
                                                      // size

    FPTYPE distsq(FPTYPE *p1_coords, FPTYPE *p2_coords, word_id_t dim);

    void compute_docs_l2sq(FPTYPE *const docs_l2sq);

    FPTYPE lloyds_iter(
        const doc_id_t num_centers, FPTYPE *centers,
        const FPTYPE *const        docs_l2sq,
        std::vector<doc_id_t> *    closest_docs = NULL,
        bool                       weighted = false,  // If true, supply weights
        const std::vector<size_t> &weights = std::vector<size_t>());

    //
    // Return last residual
    //
    FPTYPE run_lloyds(
        const doc_id_t num_centers, FPTYPE *centers,
        std::vector<doc_id_t>
            *closest_docs,  // Pass NULL if you dont want closest_docs returned
        const int                  max_reps,
        bool                       weighted = false,  // If true, supply weights
        const std::vector<size_t> &weights = std::vector<size_t>());
  };
}
