#

# Benchmark results

```sh
tile_size: 254 overlap: 1

Run on (24 X 2112 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 2048 KiB (x12)
  L3 Unified 30720 KiB (x1)
-----------------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                                         Time             CPU   Iterations
-----------------------------------------------------------------------------------------------------------------------------------
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              6.82 ms         8.67 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              6.64 ms         4.92 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              6.56 ms         5.16 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              6.31 ms         5.62 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              6.74 ms         4.77 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_mean         6.61 ms         5.83 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_median       6.64 ms         5.16 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_stddev      0.196 ms         1.62 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_cv           2.96 %         27.84 %             5

BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time              59.9 ms         57.4 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time              59.7 ms         57.3 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time              59.8 ms         57.3 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time              59.5 ms         57.3 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time              59.9 ms         57.6 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_mean         59.8 ms         57.4 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_median       59.8 ms         57.3 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_stddev      0.143 ms        0.131 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_cv           0.24 %          0.23 %             5

BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     20.1 ms         29.3 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     19.5 ms         25.5 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     19.3 ms         22.3 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     18.8 ms         31.2 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     18.6 ms         31.0 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_mean                19.3 ms         27.9 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_median              19.3 ms         29.3 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_stddev             0.616 ms         3.86 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_cv                  3.20 %         13.85 %             5


tile_size: 510 overlap: 1

Run on (24 X 2112 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 2048 KiB (x12)
  L3 Unified 30720 KiB (x1)
-----------------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                                         Time             CPU   Iterations
-----------------------------------------------------------------------------------------------------------------------------------
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              10.7 ms         11.8 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              10.2 ms         8.44 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              10.0 ms         8.20 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              10.0 ms         8.12 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              9.92 ms         7.50 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_mean         10.2 ms         8.81 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_median       10.0 ms         8.20 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_stddev      0.313 ms         1.70 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_cv           3.08 %         19.34 %             5

BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               103 ms         98.8 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               101 ms         98.8 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               102 ms         98.2 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               102 ms         98.8 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               102 ms         98.9 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_mean          102 ms         98.7 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_median        102 ms         98.8 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_stddev      0.486 ms        0.278 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_cv           0.48 %          0.28 %             5

BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     34.4 ms         44.5 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     32.8 ms         48.4 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     32.4 ms         50.9 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     32.3 ms         49.6 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     32.5 ms         50.0 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_mean                32.9 ms         48.7 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_median              32.5 ms         49.6 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_stddev             0.850 ms         2.51 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_cv                  2.59 %          5.17 %             5

tile_size: 1022 overlap: 1

Run on (24 X 2112 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x12)
  L1 Instruction 32 KiB (x12)
  L2 Unified 2048 KiB (x12)
  L3 Unified 30720 KiB (x1)
-----------------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                                         Time             CPU   Iterations
-----------------------------------------------------------------------------------------------------------------------------------
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              18.1 ms         18.6 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              17.6 ms         14.4 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              17.5 ms         14.3 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              17.2 ms         14.7 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time              17.6 ms         14.8 ms          200
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_mean         17.6 ms         15.3 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_median       17.6 ms         14.7 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_stddev      0.321 ms         1.83 ms            5
BM_dz_openslide_get_tile_jpg/1024/iterations:200/repeats:5/process_time/real_time_cv           1.82 %         11.91 %             5

BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               211 ms          207 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               212 ms          206 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               212 ms          207 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               211 ms          207 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time               212 ms          207 ms          200
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_mean          212 ms          207 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_median        212 ms          207 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_stddev      0.441 ms        0.278 ms            5
BM_dz_openslide_get_tile_png/1024/iterations:200/repeats:5/process_time/real_time_cv           0.21 %          0.13 %             5

BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     72.5 ms          222 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     71.0 ms          225 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     70.9 ms          217 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     71.0 ms          203 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time                     69.2 ms          167 ms          200
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_mean                70.9 ms          207 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_median              71.0 ms          217 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_stddev              1.18 ms         23.6 ms            5
BM_dz_qupath_get_tile/1024/iterations:200/repeats:5/process_time/real_time_cv                  1.67 %         11.41 %             5
```