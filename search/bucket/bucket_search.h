#pragma once

#include <random>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <chrono>
#include <string>
#include <cmath>

#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/ranges.h>



struct BucketIndex {
  double min_val{0};
  double max_val{0};
  double range_span{1.0};
  double inv_span{1.0};
  
  std::vector<double> slopes;
  std::vector<double> intercepts;

  std::vector<long long> bucket_start;
  std::vector<long long> counts;
};



struct SortedDataset {
  std::vector<double> data;
  BucketIndex bucket_index; 



  // 2 op, O(1)
  inline double fast_abs(double x) const {
    union {
      double d;
      uint64_t i;
    } u;
    u.d = x;                                                                              // 1
    u.i &= 0x7FFFFFFFFFFFFFFFULL;                                                         // 1
    return u.d;                                                                           // 0
  };



  // [3, 7] op,  O(1)
  inline long long find_segment(double val) const {
    const long long num_segs = static_cast<long long>(bucket_index.slopes.size());
    if (num_segs <= 1) return 0;
    if (val <= bucket_index.min_val) return 0;
    if (val >= bucket_index.max_val) return num_segs - 1;

    double normalized = (val - bucket_index.min_val) * bucket_index.inv_span;
    long long seg = static_cast<long long>(normalized * num_segs);

    if (seg < 0) return 0;
    if (seg >= num_segs) return num_segs - 1;
    return seg;
  };



  // 16 op, O(1)
  inline long long bucket_clasificator(double val) const {
    const long long ns = static_cast<long long>(bucket_index.slopes.size());
    long long seg = static_cast<long long>((val - bucket_index.min_val) * bucket_index.inv_span * ns);
    seg = seg < 0 ? 0 : (seg >= ns ? ns - 1 : seg);
    long long b = static_cast<long long>(bucket_index.slopes[seg] * val + bucket_index.intercepts[seg]);
    const long long mb = static_cast<long long>(bucket_index.bucket_start.size()) - 2;
    return b < 0 ? 0 : (b > mb ? mb : b);
  };



#define CLASSIFY_FAST(val, min_val, inv_span, num_segs, slopes_ptr, intercepts_ptr, max_b_idx, b_out) \
  do { \
    double v_ = (val); \
    double norm_ = (v_ - (min_val)) * (inv_span); \
    long long seg_ = static_cast<long long>(norm_ * (num_segs)); \
    seg_ = (seg_ < 0) ? 0 : (seg_ >= (num_segs) ? ((num_segs) - 1) : seg_); \
    double est_ = (slopes_ptr)[seg_] * v_ + (intercepts_ptr)[seg_]; \
    long long b_macro_ = static_cast<long long>(est_); \
    (b_out) = (b_macro_ < 0) ? 0 : (b_macro_ > (max_b_idx) ? (max_b_idx) : b_macro_); \
  } while(0)



  // f(n, b, s) = 61n + 24s + 6b - 73 op, O(n)
  void build_index(long long num_buckets = 0, long long target_segment_size = 64) {
    if (data.empty()) return;

    const long long n = static_cast<long long>(data.size());
    if (num_buckets == 0) num_buckets = n;

    const double* __restrict d_ptr = data.data();

    double min_v = d_ptr[0];
    double max_v = d_ptr[0];
    for (long long i = 1; i < n; ++i) {
      double v = d_ptr[i];
      if (v < min_v) min_v = v;
      if (v > max_v) max_v = v;
    };

    bucket_index.min_val = min_v;
    bucket_index.max_val = max_v;
    bucket_index.range_span = max_v - min_v;
    if (bucket_index.range_span <= 0.0) bucket_index.range_span = 1.0;

    const double inv_span = 1.0 / bucket_index.range_span;

    long long num_segments = n / target_segment_size;
    if (num_segments < 1000) num_segments = 1000;
    if (num_segments > 65536) num_segments = 65536;

    std::vector<long long> seg_counts(num_segments, 0);
    long long* __restrict seg_cnt_ptr = seg_counts.data();
    const double scale_seg = static_cast<double>(num_segments) * inv_span;
    const long long max_seg_idx = num_segments - 1;

    long long i = 0;
    for (; i <= n - 4; i += 4) {
      long long s0 = static_cast<long long>((d_ptr[i]   - min_v) * scale_seg);
      long long s1 = static_cast<long long>((d_ptr[i+1] - min_v) * scale_seg);
      long long s2 = static_cast<long long>((d_ptr[i+2] - min_v) * scale_seg);
      long long s3 = static_cast<long long>((d_ptr[i+3] - min_v) * scale_seg);

      s0 = (s0 < 0) ? 0 : (s0 > max_seg_idx ? max_seg_idx : s0);
      s1 = (s1 < 0) ? 0 : (s1 > max_seg_idx ? max_seg_idx : s1);
      s2 = (s2 < 0) ? 0 : (s2 > max_seg_idx ? max_seg_idx : s2);
      s3 = (s3 < 0) ? 0 : (s3 > max_seg_idx ? max_seg_idx : s3);

      seg_cnt_ptr[s0]++;
      seg_cnt_ptr[s1]++;
      seg_cnt_ptr[s2]++;
      seg_cnt_ptr[s3]++;
    };

    for (; i < n; ++i) {
      long long s = static_cast<long long>((d_ptr[i] - min_v) * scale_seg);
      s = (s < 0) ? 0 : (s > max_seg_idx ? max_seg_idx : s);
      seg_cnt_ptr[s]++;
    };

    bucket_index.slopes.resize(num_segments);
    bucket_index.intercepts.resize(num_segments);

    double seg_width = bucket_index.range_span / num_segments;
    double scale_to_buckets = static_cast<double>(num_buckets) / static_cast<double>(n);
    long long cumulative_elements = 0;

    for (long long s = 0; s < num_segments; ++s) {
      double x_first = min_v + s * seg_width;
      double x_last = (s == num_segments - 1) ? max_v : (min_v + (s + 1) * seg_width);

      double y_start = static_cast<double>(cumulative_elements) * scale_to_buckets;
      cumulative_elements += seg_cnt_ptr[s];
      double y_end = static_cast<double>(cumulative_elements) * scale_to_buckets;

      double dx = x_last - x_first;
      if (dx > 1e-9) {
        double slope = (y_end - y_start) / dx;
        bucket_index.slopes[s] = slope;
        bucket_index.intercepts[s] = y_start - slope * x_first;
      } else {
        bucket_index.slopes[s] = 0.0;
        bucket_index.intercepts[s] = y_start;
      };
    };

    bucket_index.bucket_start.assign(num_buckets + 1, 0);
    bucket_index.counts.assign(n, 0);

    long long* __restrict start_ptr = bucket_index.bucket_start.data();
    long long* __restrict items_ptr = bucket_index.counts.data();
    const double* __restrict slopes_ptr = bucket_index.slopes.data();
    const double* __restrict intercepts_ptr = bucket_index.intercepts.data();
    const long long max_b_idx = num_buckets - 1;

    i = 0;
    for (; i <= n - 8; i += 8) {
      long long b0, b1, b2, b3, b4, b5, b6, b7;
      CLASSIFY_FAST(d_ptr[i],   min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b0);
      CLASSIFY_FAST(d_ptr[i+1], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b1);
      CLASSIFY_FAST(d_ptr[i+2], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b2);
      CLASSIFY_FAST(d_ptr[i+3], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b3);
      CLASSIFY_FAST(d_ptr[i+4], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b4);
      CLASSIFY_FAST(d_ptr[i+5], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b5);
      CLASSIFY_FAST(d_ptr[i+6], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b6);
      CLASSIFY_FAST(d_ptr[i+7], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b7);

      start_ptr[b0]++; start_ptr[b1]++; start_ptr[b2]++; start_ptr[b3]++;
      start_ptr[b4]++; start_ptr[b5]++; start_ptr[b6]++; start_ptr[b7]++;
    };

    for (; i < n; ++i) {
      long long b_id;
      CLASSIFY_FAST(d_ptr[i], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b_id);
      start_ptr[b_id]++;
    };

    long long current_pos = 0;
    for (long long b = 0; b < num_buckets; ++b) {
      current_pos += start_ptr[b];
      start_ptr[b] = current_pos;
    };
    start_ptr[num_buckets] = n;

    i = 0;
    for (; i <= n - 4; i += 4) {
      long long b0, b1, b2, b3;
      CLASSIFY_FAST(d_ptr[i],   min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b0);
      CLASSIFY_FAST(d_ptr[i+1], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b1);
      CLASSIFY_FAST(d_ptr[i+2], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b2);
      CLASSIFY_FAST(d_ptr[i+3], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b3);

      items_ptr[--start_ptr[b0]] = i;
      items_ptr[--start_ptr[b1]] = i + 1;
      items_ptr[--start_ptr[b2]] = i + 2;
      items_ptr[--start_ptr[b3]] = i + 3;
    };

    for (; i < n; ++i) {
      long long b_id;
      CLASSIFY_FAST(d_ptr[i], min_v, inv_span, num_segments, slopes_ptr, intercepts_ptr, max_b_idx, b_id);
      items_ptr[--start_ptr[b_id]] = i;
    };

    auto by_value = [d_ptr](long long a, long long b) { return d_ptr[a] < d_ptr[b]; };

    for (long long b = 0; b < num_buckets; ++b) {
      long long s = start_ptr[b];
      long long cnt = start_ptr[b + 1] - s;

      if (cnt == 2) {
        if (d_ptr[items_ptr[s]] > d_ptr[items_ptr[s + 1]]) {
          std::swap(items_ptr[s], items_ptr[s + 1]);
        };
      } else if (cnt > 2 && cnt <= 32) {
        for (long long j = s + 1; j < s + cnt; ++j) {
          long long key = items_ptr[j];
          double key_val = d_ptr[key];
          long long k = j - 1;
          while (k >= s && d_ptr[items_ptr[k]] > key_val) {
            items_ptr[k + 1] = items_ptr[k];
            --k;
          };
          items_ptr[k + 1] = key;
        };
      } else if (cnt > 32) {
        std::sort(items_ptr + s, items_ptr + s + cnt, by_value);
      };
    };
    
    bucket_index.inv_span = 1.0 / bucket_index.range_span;
  };



  // 5 op, O(1)
  void clear() {
    data.clear();
    bucket_index.slopes.clear();
    bucket_index.intercepts.clear();
    bucket_index.bucket_start.clear();
    bucket_index.counts.clear();
  };



  inline long long search_window(double target, long long lo, long long hi) const {
    const double* d = data.data();
    const long long* it = bucket_index.counts.data();
    const long long n = static_cast<long long>(data.size());

    long long pos;
    if (hi - lo <= 16) {
      long long c = 0;
      for (long long j = lo; j < hi; ++j) c += (d[it[j]] < target);
      pos = lo + c;
    } else {
      pos = std::lower_bound(it + lo, it + hi, target,
        [d](long long idx, double t) { return d[idx] < t; }) - it;
    };

    if (pos == 0) return 0;
    if (pos == n) return n - 1;
    const double dl = target - d[it[pos - 1]];
    const double dh = d[it[pos]] - target;
    return (dh < dl) ? pos : pos - 1;
  };



  // 54 + klogk op, O(1) + O(log2(k)) 
  long long find_nearest_bucketed(double target) const {
    if (data.empty()) return -1;
    const long long* st = bucket_index.bucket_start.data();
    const long long nb = static_cast<long long>(bucket_index.bucket_start.size()) - 1;
    const long long b = bucket_clasificator(target);
    return search_window(target, st[b > 0 ? b - 1 : 0], st[b < nb - 1 ? b + 2 : nb]);
  };



  void print_params() const {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== Learned Index Parameters (O(1) Piecewise Linear) ===\n");
    fmt::print(fmt::fg(fmt::color::green),  "  Dataset Size:        {}\n", data.size());
    fmt::print(fmt::fg(fmt::color::yellow), "  Min / Max Value:     {:.4f} / {:.4f}\n", bucket_index.min_val, bucket_index.max_val);
    fmt::print(fmt::fg(fmt::color::yellow), "  Range Span:          {:.4f}\n", bucket_index.range_span);
    fmt::print(fmt::fg(fmt::color::yellow), "  Segments Count:      {}\n", bucket_index.slopes.size());
    fmt::print(fmt::fg(fmt::color::magenta), "  Total Buckets:       {}\n", bucket_index.bucket_start.size());
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, "===============================================\n\n");
  };



  void analyze_index_stats() const {
    if (bucket_index.bucket_start.size() < 2) {
      fmt::println(fg(fmt::color::red), "Index is empty or not built!");
      return;
    };

    const long long total_buckets = static_cast<long long>(bucket_index.bucket_start.size()) - 1;
    long long non_empty_buckets = 0;
    long long max_bucket_size = 0;
    long long sum_bucket_size = 0;
    long long empty_buckets = 0;

    for (long long b = 0; b < total_buckets; ++b) {
      const long long cnt = bucket_index.bucket_start[b + 1] - bucket_index.bucket_start[b];
      if (cnt > 0) {
        non_empty_buckets++;
        sum_bucket_size += cnt;
        if (cnt > max_bucket_size) {
          max_bucket_size = cnt;
        }
      } else {
        empty_buckets++;
      };
    };

    double avg_bucket_size = non_empty_buckets > 0 ? static_cast<double>(sum_bucket_size) / non_empty_buckets : 0.0;
    long long worst_case_elements_to_scan = max_bucket_size * 3;

    fmt::println(fg(fmt::color::yellow), "\n=== Index Quality & Query Cost Analysis ===");
    fmt::println("  Total Buckets:       {}", total_buckets);
    fmt::println("  Non-empty Buckets:   {} ({:.2f}%)", non_empty_buckets, (double)non_empty_buckets / total_buckets * 100.0);
    fmt::println("  Empty Buckets:       {}", empty_buckets);
    fmt::println("  Avg Elements/Bucket: {:.2f}", avg_bucket_size);
    fmt::println("  Max Bucket Size:     {} elements", max_bucket_size);
    fmt::println(fg(fmt::color::green), "  Max Elements Scanned per Query (Worst Case ±1 window): ~{}", worst_case_elements_to_scan);
    fmt::println("===========================================\n");
  };



  void graph_bucket_clasificator_terminal(int width = 80, int height = 24) const {
    const auto& index = bucket_index;
    if (index.slopes.empty() || index.intercepts.empty() || index.bucket_start.size() < 2) {
      fmt::print(fmt::fg(fmt::color::red), "Index is empty, cannot draw graph.\n");
      return;
    };
  
    const double min_val = index.min_val;
    const double max_val = index.max_val;
    const double span = index.range_span;
    const long long max_b_idx = static_cast<long long>(index.bucket_start.size()) - 2;
  
    if (max_b_idx <= 0 || span <= 0.0) {
      fmt::print(fmt::fg(fmt::color::red), "Invalid index range for plotting.\n");
      return;
    };
  
    std::vector<std::string> canvas(height, std::string(width, ' '));
    std::vector<long long> y_samples(width, 0);
  
    for (int col = 0; col < width; ++col) {
      double ratio = (width > 1) ? static_cast<double>(col) / (width - 1) : 0.0;
      double x = min_val + ratio * span;
      long long b_id = bucket_clasificator(x);
      y_samples[col] = b_id;
    };
  
    for (int col = 0; col < width; ++col) {
      int row = static_cast<int>(std::round((static_cast<double>(y_samples[col]) / max_b_idx) * (height - 1)));
      row = std::clamp(row, 0, height - 1);
      int screen_y = (height - 1) - row; 
  
      canvas[screen_y][col] = '*';
  
      if (col > 0) {
        int prev_row = static_cast<int>(std::round((static_cast<double>(y_samples[col - 1]) / max_b_idx) * (height - 1)));
        prev_row = std::clamp(prev_row, 0, height - 1);
        int prev_screen_y = (height - 1) - prev_row;
  
        int y_start = std::min(screen_y, prev_screen_y);
        int y_end = std::max(screen_y, prev_screen_y);
        for (int y = y_start + 1; y < y_end; ++y) {
          if (canvas[y][col] == ' ') canvas[y][col] = '|';
        };
      };
    };
  
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, "\n=== Classifier Graph: bucket_clasificator(x) [Segments: {}] ===\n", index.slopes.size());
  
    for (int r = 0; r < height; ++r) {
      double y_label_val = static_cast<double>(max_b_idx) * (height - 1 - r) / (height - 1);
      fmt::print(fmt::fg(fmt::color::yellow), "{:>10.0f} |", y_label_val);
      fmt::print(fmt::fg(fmt::color::green), "{}\n", canvas[r]);
    };
  
    fmt::print(fmt::fg(fmt::color::yellow), "{:>10} +{}\n", "", std::string(width, '-'));
    
    double mid_val = min_val + span * 0.5;
    std::string x_labels = fmt::format("{:<16.2f}{:^48.2f}{:>16.2f}", min_val, mid_val, max_val);
    fmt::print(fmt::fg(fmt::color::cyan), "{:>12}{}\n\n", "", x_labels);
  };
};

