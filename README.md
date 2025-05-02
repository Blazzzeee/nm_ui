## Benchmarks 
### Python3 (Without render overhead) </br>
 Mean [ms]   | Min [ms] | Max [ms] | Relative |</br>
 191.5 ± 3.5 | 182.5 | 199.4 | 1.00 |</br></br>
### C (Without render overhead)</br>
 Mean [ms] | Min [ms] | Max [ms] | Relative |</br>
| 11.3 ± 1.0 | 9.7 | 14.7 | 1.00 |

### Python3 (With Render Overhead)
#### Using inbuilt timing methods  
**Elapsed time:** `1.487729` seconds  

#### Python code used for profiling:
```python
profiler = cProfile.Profile()
profiler.enable()

main()

profiler.disable()
end = time.time()
print("Main finished")

profiler.print_stats()
print(f"Elapsed time: {end - start:.6f} seconds")
```
### C (With Render Overhead)
#### Using inbuilt timing methods  
**Elapsed time:** `0.007561` seconds

```c
int main() {
    clock_t start_time, end_time;

    start_time = clock();

    StartEventLoop();

    Terminate(global_ctx);

    end_time = clock();

    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("Elapsed time: %f seconds\n", elapsed_time);

    return 0;
}

```
