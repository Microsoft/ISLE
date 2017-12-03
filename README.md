This repository provides code SVD- and Importance sampling-based algorithms for large scale topic modeling.

# Build

Both Linux and Windows 10 builds use Intel(R) MKL(R) library. We used 2017 version; other versions might work as well.
Suppose you have installed MKL at <MKL_ROOT>. In Linux, this is typically `/opt/intel/mkl`.
In Windows 10, this is typically `C:\Program Files (x86)\IntelSWTools\compilers_and_libraries_2017\windows\mkl`

## Linux
```
export LD_LIBRARY_PATH=<MKL_ROOT>/lib/intel64/:.
make -j
```
This should generate two executables `trainFromFile` and `inferFromFile` in the root directory.


## Compilation instructions for VS2015

Open `win\ISLE.sln` in VS2015, and build the `ISLETrain` and `ISLEInfer` projects.  

If there is a problem with this, you could configure project file properties as follows:
* Under VC++ Directories >
  * To Include directories, add `<ISLE_ROOT>` and `<ISLE_ROOT>\include`. This will include `include`,  `Eigen` and `Spectra` directories.
  * To Library Directories, add `<MKL_ROOT>\lib\intel64_win`.

* Under C/C++ >
  * Enable optimizations, and disable runtime and SCL checks (this conflicts with turning on optimizations).
  * Code Generation > Enable Parallel Code
  * Languages > Enable OpenMP support

* Under 'Intel Performance Libraries' >
  * Enable 'Parallel' option.
  * Enable MKL_ILP64 option for 8-byte MKL_INTs.

* Under Linker > Input > Additional dependencies:
  * `<MKL_ROOT>\lib\intel64_win\mkl_core.lib`
  * `<MKL_ROOT>\lib\intel64_win\mkl_intel_lp64.lib`
  * `<MKL_ROOT>\lib\intel64_win\mkl_sequential.lib`

# Training on a dataset

1. Create a `<tdf_file>` which has one `<doc_id> <word_id> <frequency>` entry per line.
   * The `<doc_id>` entries are 1-based and range between 1 and <num_docs>. 
   * The `<word_id>` enties are 1-based and range between 1 and <vocab_size>.
   * Let `<max_entries>` be the number of entries (or lines) in this file.

2. Create a `<vocab_file>` file with mapping from word id to the word; the i-th line of the file is the word with word_id=i.

3. Run 
 ```trainFromFile <tdf_file> <vocab_file> <output_dir> <vocab_size> <num_docs> <max_entries> <num_topics> <sample(0/1)> <sample_rate>```
   * Here `<num_topics>` is the number of topics you want to recover from the `<tdf_file>`.
   * If the dataset is too large and you wish to use importance sampling, set `<sample>` to 1 (otherwise 0).
   * When `<sample>` is enabled by setting it to 1, you can specify the sampling rate with `<sample_rate>`. For example, 0.1.
   * The output will be stored in a log directory under `<output_dir>`

# Inference for a dataset using the trained model



# Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
