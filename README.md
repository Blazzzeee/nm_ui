## Benchmarks 
### Python3 (Without render overhead) </br>
 Mean [ms]   | Min [ms] | Max [ms] | Relative |</br>
 191.5 ± 3.5 | 182.5 | 199.4 | 1.00 |</br></br>
### C (Without render overhead)</br>
 Mean [ms] | Min [ms] | Max [ms] | Relative |</br>
| 11.3 ± 1.0 | 9.7 | 14.7 | 1.00 |

### Python3 (with render overhead)</br>
#### Using inbuilt timing methods
Elapsed time: 1.487729 seconds
Python code user for profiling 
'''
if __name__ == '__main__':
    start = time.time()
    print("Main started")

    # Profile the main function
    profiler = cProfile.Profile()
    profiler.enable()

    main()

    profiler.disable()
    end = time.time()
    print("Main finished")

    # Print profiling results
    profiler.print_stats()

    print(f"Elapsed time: {end - start:.6f} seconds")
'''
### C (With render overhead)</br>
#### Using inbuilt timing methods
Elapsed time: 0.011355 seconds
