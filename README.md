# TSan-SPD3 Data Race Detector
TSan-SPD3 is a dynamic race detector for OpenMP programs. It is implemented on top of the ThreadSanitizer (TSan), 
reusing the majority of TSan's infrastructure, e.g., shadow memory and bug report. TSan-SPD3 leverages the 
[Scalable Precise Dynamic Datarace Detection (SPD3)](https://dl.acm.org/doi/pdf/10.1145/2345156.2254127) algorithm, 
which was proposed by Raghavan Raman et al. in PLDI'12. Although initially designed for async-finish task parallelism,
SPD3 can be applied to OpenMP programs. TSan-SPD3 can precisely identify all potential data races under the given input 
if these programs only use the subset of supported OpenMP constructs; otherwise, TSan-SPD3 may report false positives 
due to its incapability of encoding happens-before relations related to those unsupported constructs. In the following 
section, we list the details of OpenMP constructs handling by TSan-SPD3.

| **Constructs** | **Supported?** |
|---|---|
| `parallel` | :heavy_check_mark: |
| `for` | :heavy_check_mark: |
| `single` | :heavy_check_mark: |
| `master/masked` | :heavy_check_mark: |
| `atomic` | :heavy_check_mark: |
| `task` | :heavy_check_mark: |
| `taskgroup` | :heavy_check_mark: |
| `taskwait` | :heavy_check_mark: |
| `taskloop` | :heavy_check_mark: |
| `barrier` | :heavy_check_mark: |
| `critical` | :heavy_multiplication_x: |
| `simd` | :heavy_multiplication_x: |
| `target` | :heavy_multiplication_x: |

| **Unsupported clauses for `parallel for`** |
|---|
| `ordered` |
| `reduction` |
| `schedule` |

| **Unsupported clauses for `task`** |
|---|
| `depend` |

## Install TSan-SPD3
### Prerequisite: install the bootstrapping compiler clang/llvm 15
1. Retrieve the `clang+llvm-15*` package from the [official repository](https://github.com/llvm/llvm-project/releases/tag/llvmorg-15.0.0).
2. Unfold the package and set the following environment variables.


    `export PATH="<UNFOLDED_LLVM_DIR>/bin:$PATH"`
    
    `export LD_LIBRARY_PATH="<UNFOLDED_LLVM_DIR>/lib:$LD_LIBRARY_PATH"`

### Retrieve TSan-SPD3's repository and configure `cmake`
Replace `<BUILD_DIR>`, `<INSTALL_DIR>`, and `<REPO_DIR>` before executing following commands.

    git clone https://github.com/lechenyu/TSan-spd3.git <REPO_DIR>
    mkdir <BUILD_DIR> && cd <BUILD_DIR>
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> \
      -DLLVM_ENABLE_LIBCXX=ON \
      -DLLVM_LIT_ARGS="-sv -j12" \
      -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
      -DLIBOMPTARGET_ENABLE_DEBUG=ON \
      -DLLVM_ENABLE_PROJECTS="clang;compiler-rt;libcxxabi;libcxx;libunwind;clang-tools-extra;openmp" \
      -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
      -DLLVM_INSTALL_UTILS=ON \
      -DLIBOMPTARGET_BUILD_CUDA_PLUGIN=False \
      -DLIBOMPTARGET_BUILD_AMDGPU_PLUGIN=False \
      <REPO_DIR>

### Build with Ninja and install
    cd <BUILD_DIR> && ninja -j 10 install
    export PATH="<INSTALL_DIR>/bin:$PATH"
    export LD_LIBRARY_PATH="<INSTALL_DIR>/lib:$LD_LIBRARY_PATH"

## Use TSan-SPD3
The usage of TSan-SPD3 is similar to TSan/[Archer](https://github.com/llvm/llvm-project/tree/main/openmp/tools/archer).
To compile an OpenMP program with TSan-SPD3 enabled, the extra flag`-fsanitize=thread` should be set on the command line.

    clang -O3 -g -fopenmp -fsanitize=thread app.c
    clang++ -O3 -g -fopenmp -fsanitize=thread app.cpp

To avoid false alerts due to the OpenMP runtime implementation, set the TSan option `ignore_noninstrumented_modules` to `1`.

    export TSAN_OPTIONS="ignore_noninstrumented_modules=1"

## Evaluation Results on [DataRaceBench](https://github.com/LLNL/dataracebench)
This is the evaluation results of the latest TSan-SPD3. These results may change in the future since we are actively updating 
TSan-SPD3 to tackle more OpenMP constructs.

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">

<colgroup>
<col  class="org-left" />
<col  class="org-right" />
</colgroup>

<tbody>
<tr>
<td class="org-left">False Positive</td>
<td class="org-right">30</td>
</tr>
<tr>
<td class="org-left">False Negative</td>
<td class="org-right">18</td>
</tr>
<tr>
<td class="org-left">True Positive</td>
<td class="org-right">72</td>
</tr>
<tr>
<td class="org-left">True Negative</td>
<td class="org-right">59</td>
</tr>
<tr>
<td class="org-left">Timeout</td>
<td class="org-right">2</td>
</tr>
</tbody>
</table>
