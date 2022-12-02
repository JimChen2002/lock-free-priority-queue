/**
 * @file benchmark.cpp
 * @brief File for benchmarking different priority queues
 * @date 2022-12-01
 *
 * program usage:
 * ./benchmark -n <NUM_THREADS> -i <TEST DURATION (in seconds)> -w <WORKLOAD TYPE>
 * 
 * 2 kinds of workloads: Uniform and DES (discrete event simulation)
 * https://github.com/jonatanlinden/PR/blob/master/perf_meas.c
 * 
 * Step 1: run experiment, measure # of ops / sec 
 * Step 2: save results to csv
 *
 */