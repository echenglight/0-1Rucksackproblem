#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>  // 用于 memcpy
#include <stdbool.h> // 用于 bool 类型

// --- 算法限制常量 ---
#define MAX_N_FOR_BRUTEFORCE 31
#define MAX_N_FOR_BACKTRACKING 31
// #define MAX_NC_FOR_DP 200000000 // DP skip condition removed as per request

// --- 特殊返回值，表示执行时间状态 ---
#define TIME_SKIPPED -1.0
#define TIME_ERROR -2.0 // 例如内存分配失败

// --- 物品结构体 ---
typedef struct
{
  int id;
  int weight;
  int value;
  double ratio;
} Item;

// --- 全局变量存储最优解 (仅用于蛮力法和回溯法内部) ---
int *g_best_selection_backtracking = NULL;
int g_max_value_backtracking = 0;
int g_best_weight_backtracking = 0;
int g_best_item_count_backtracking = 0;

int *g_best_selection_bruteforce = NULL;
int g_max_value_bruteforce = 0;
int g_best_weight_bruteforce = 0;
int g_best_item_count_bruteforce = 0;

// --- 用于存储所有运行的时间信息 ---
typedef struct
{
  int n;
  int c;
  double times[4]; // 0: BF, 1: DP, 2: Greedy, 3: BT
} TimingInfo;
TimingInfo *all_timing_data = NULL;
int timing_data_count = 0;
int timing_data_capacity = 0;

// --- 辅助函数：打印选中的物品和结果 (不再打印时间) ---
void print_solution_details(const char *method_name, Item *all_items, int n_items_total, int *selected_item_indices, int num_selected, int total_value, int total_weight)
{
  printf("\n--- %s (结果详情) ---\n", method_name);
  if (num_selected == -1)
  { // 特殊标记，表示算法未成功找到解（例如，因错误跳过）
    printf("   由于错误，未能生成结果。\n");
  }
  else if (num_selected == 0)
  {
    printf("   未选中任何物品。\n");
  }
  else
  {
    printf("选中的物品:\n");
    for (int i = 0; i < num_selected; ++i)
    {
      int item_idx = selected_item_indices[i];
      if (item_idx >= 0 && item_idx < n_items_total)
      {
        printf("   编号: %d, 重量: %d, 价值: %d\n", all_items[item_idx].id, all_items[item_idx].weight, all_items[item_idx].value);
      }
    }
  }
  if (num_selected != -1)
  {
    printf("总重量: %d\n", total_weight);
    printf("总价值: %d\n", total_value);
  }
  printf("-------------------------------------\n");
}

// --- 1. 蛮力算法 ---
void knapsack_bruteforce_recursive(Item *items, int n, int capacity, int index,
                                   int current_weight, int current_value,
                                   int *current_selection, int current_item_count)
{
  if (index == n)
  {
    if (current_weight <= capacity && current_value > g_max_value_bruteforce)
    {
      g_max_value_bruteforce = current_value;
      g_best_weight_bruteforce = current_weight;
      g_best_item_count_bruteforce = current_item_count;
      for (int i = 0; i < current_item_count; ++i)
      {
        g_best_selection_bruteforce[i] = current_selection[i];
      }
    }
    return;
  }
  knapsack_bruteforce_recursive(items, n, capacity, index + 1,
                                current_weight, current_value,
                                current_selection, current_item_count);
  if (current_weight + items[index].weight <= capacity)
  {
    current_selection[current_item_count] = index;
    knapsack_bruteforce_recursive(items, n, capacity, index + 1,
                                  current_weight + items[index].weight,
                                  current_value + items[index].value,
                                  current_selection, current_item_count + 1);
  }
}

double solve_bruteforce(Item *items, int n, int capacity)
{
  const char *method_name = "蛮力法";
  if (n > MAX_N_FOR_BRUTEFORCE)
  {
    printf("\n--- %s ---\n", method_name);
    printf("跳过执行：N=%d 个物品 (对于蛮力法来说数量过大，上限 %d)。\n", n, MAX_N_FOR_BRUTEFORCE);
    printf("-------------------------------------\n");
    return TIME_SKIPPED;
  }

  g_max_value_bruteforce = 0;
  g_best_weight_bruteforce = 0;
  g_best_item_count_bruteforce = 0;
  if (g_best_selection_bruteforce != NULL)
  {
    free(g_best_selection_bruteforce);
    g_best_selection_bruteforce = NULL;
  }
  g_best_selection_bruteforce = (int *)malloc(n * sizeof(int));
  if (!g_best_selection_bruteforce)
  {
    perror("为蛮力法最佳选择分配内存失败");
    print_solution_details(method_name, items, n, NULL, -1, 0, 0); // 指示错误
    return TIME_ERROR;
  }

  int *current_selection_bf = (int *)malloc(n * sizeof(int));
  if (!current_selection_bf)
  {
    perror("为蛮力法当前选择分配内存失败");
    // g_best_selection_bruteforce 理论上应被释放，但为简化，依赖main中最后的free
    print_solution_details(method_name, items, n, NULL, -1, 0, 0);
    return TIME_ERROR;
  }

  clock_t start_time = clock();
  knapsack_bruteforce_recursive(items, n, capacity, 0, 0, 0, current_selection_bf, 0);
  clock_t end_time = clock();
  double time_taken = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;

  print_solution_details(method_name, items, n, g_best_selection_bruteforce, g_best_item_count_bruteforce, g_max_value_bruteforce, g_best_weight_bruteforce);

  free(current_selection_bf);
  // g_best_selection_bruteforce将在main函数末尾或下次调用solve_bruteforce时被释放
  return time_taken;
}

// --- 2. 动态规划算法 ---
double solve_dp(Item *items, int n, int capacity)
{
  const char *method_name = "动态规划";
  // Removed skip condition as per request
  /*
  if ((long long)n * capacity > MAX_NC_FOR_DP)
  {
    printf("\n--- %s ---\n", method_name);
    printf("跳过执行：N=%d, C=%d (N*C = %lld 对于动态规划来说过大，上限 %lld)。\n", n, capacity, (long long)n * capacity, (long long)MAX_NC_FOR_DP);
    printf("-------------------------------------\n");
    return TIME_SKIPPED;
  }
  */
  printf("\n--- %s (尝试执行 N=%d, C=%d, N*C=%lld) ---\n", method_name, n, capacity, (long long)n * capacity);

  int **dp = (int **)malloc((n + 1) * sizeof(int *));
  if (!dp)
  {
    perror("为DP表行指针分配内存失败");
    print_solution_details(method_name, items, n, NULL, -1, 0, 0);
    return TIME_ERROR;
  }
  for (int i = 0; i <= n; i++)
  {
    dp[i] = (int *)malloc((capacity + 1) * sizeof(int));
    if (!dp[i])
    {
      perror("为DP表列分配内存失败");
      fprintf(stderr, "DP表分配失败: N=%d, C=%d, 尝试为dp[%d]分配 (capacity+1)=%d 个int.\n", n, capacity, i, capacity + 1);
      for (int k = 0; k < i; ++k)
        free(dp[k]);
      free(dp);
      print_solution_details(method_name, items, n, NULL, -1, 0, 0);
      return TIME_ERROR;
    }
  }

  clock_t start_time = clock();

  for (int i = 0; i <= n; i++)
  {
    for (int w = 0; w <= capacity; w++)
    {
      if (i == 0 || w == 0)
        dp[i][w] = 0;
      else if (items[i - 1].weight <= w)
        dp[i][w] = (items[i - 1].value + dp[i - 1][w - items[i - 1].weight] > dp[i - 1][w]) ? (items[i - 1].value + dp[i - 1][w - items[i - 1].weight]) : dp[i - 1][w];
      else
        dp[i][w] = dp[i - 1][w];
    }
  }

  int max_value_dp = dp[n][capacity];
  int current_weight_dp = 0;
  int *selected_items_indices_dp = (int *)malloc(n * sizeof(int));
  if (!selected_items_indices_dp)
  {
    perror("为DP选中物品列表分配内存失败");
    for (int i_clean = 0; i_clean <= n; i_clean++)
      free(dp[i_clean]);
    free(dp);
    print_solution_details(method_name, items, n, NULL, -1, 0, 0);
    return TIME_ERROR;
  }
  int count_dp = 0;
  int w_trace = capacity;
  for (int i_trace = n; i_trace > 0 && max_value_dp > 0; i_trace--)
  {
    if (dp[i_trace][w_trace] != dp[i_trace - 1][w_trace])
    {
      selected_items_indices_dp[count_dp++] = i_trace - 1;
      current_weight_dp += items[i_trace - 1].weight;
      w_trace -= items[i_trace - 1].weight;
    }
  }
  clock_t end_time = clock();
  double time_taken = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;

  print_solution_details(method_name, items, n, selected_items_indices_dp, count_dp, dp[n][capacity], current_weight_dp);

  free(selected_items_indices_dp);
  for (int i = 0; i <= n; i++)
    free(dp[i]);
  free(dp);
  return time_taken;
}

// --- 3. 贪心算法 ---
int compareItems(const void *a, const void *b)
{
  Item *itemA = (Item *)a;
  Item *itemB = (Item *)b;
  if (itemB->ratio > itemA->ratio)
    return 1;
  if (itemB->ratio < itemA->ratio)
    return -1;
  return 0;
}

double solve_greedy(Item *original_items, int n, int capacity)
{
  const char *method_name = "贪心法 (按价值/重量比，非最优解)";
  Item *items_copy = (Item *)malloc(n * sizeof(Item));
  if (!items_copy)
  {
    perror("为贪心法复制物品分配内存失败");
    print_solution_details(method_name, original_items, n, NULL, -1, 0, 0);
    return TIME_ERROR;
  }
  memcpy(items_copy, original_items, n * sizeof(Item));

  for (int i = 0; i < n; i++)
  {
    if (items_copy[i].weight > 0)
      items_copy[i].ratio = (double)items_copy[i].value / items_copy[i].weight;
    else
      items_copy[i].ratio = items_copy[i].value > 0 ? 1e9 : 0; // Handle weight 0 items
  }
  qsort(items_copy, n, sizeof(Item), compareItems);

  clock_t start_time = clock();
  int current_weight_greedy = 0;
  int total_value_greedy = 0;
  int *selected_items_indices_greedy = (int *)malloc(n * sizeof(int));
  if (!selected_items_indices_greedy)
  {
    perror("为贪心法选中物品列表分配内存失败");
    free(items_copy);
    print_solution_details(method_name, original_items, n, NULL, -1, 0, 0);
    return TIME_ERROR;
  }
  int count_greedy = 0;
  for (int i = 0; i < n; i++)
  {
    if (current_weight_greedy + items_copy[i].weight <= capacity)
    {
      current_weight_greedy += items_copy[i].weight;
      total_value_greedy += items_copy[i].value;
      // Find original index
      bool found = false;
      for (int j = 0; j < n; ++j)
      {
        // This relies on IDs being unique if items can have same weight/value
        // For this problem, items are distinct by their original array position.
        // If IDs are not unique or items are perfect duplicates, this might pick "a" duplicate.
        // Here, items_copy[i].id refers to the original ID.
        if (original_items[j].id == items_copy[i].id)
        {
          selected_items_indices_greedy[count_greedy++] = j;
          found = true;
          break;
        }
      }
      if (!found)
      { /* Should not happen if IDs are preserved and unique */
        fprintf(stderr, "Error: Could not find original item for greedy selection (ID: %d)\n", items_copy[i].id);
      }
    }
  }
  clock_t end_time = clock();
  double time_taken = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;

  print_solution_details(method_name, original_items, n, selected_items_indices_greedy, count_greedy, total_value_greedy, current_weight_greedy);

  free(selected_items_indices_greedy);
  free(items_copy);
  return time_taken;
}

// --- 4. 回溯算法 ---
void knapsack_backtracking_recursive(Item *items, int n, int capacity, int index,
                                     int current_weight, int current_value,
                                     bool *current_selection_flags)
{
  if (current_weight > capacity)
    return; // Pruning step
  if (index == n)
  {
    if (current_value > g_max_value_backtracking)
    {
      g_max_value_backtracking = current_value;
      g_best_weight_backtracking = current_weight;
      g_best_item_count_backtracking = 0;
      for (int i = 0; i < n; ++i)
      {
        if (current_selection_flags[i])
        {
          g_best_selection_backtracking[g_best_item_count_backtracking++] = i;
        }
      }
    }
    return;
  }

  // Decision: Don't include items[index]
  current_selection_flags[index] = false;
  knapsack_backtracking_recursive(items, n, capacity, index + 1,
                                  current_weight, current_value,
                                  current_selection_flags);

  // Decision: Include items[index] (if possible)
  if (current_weight + items[index].weight <= capacity)
  {
    current_selection_flags[index] = true;
    knapsack_backtracking_recursive(items, n, capacity, index + 1,
                                    current_weight + items[index].weight,
                                    current_value + items[index].value,
                                    current_selection_flags);
    current_selection_flags[index] = false; // Backtrack: reset flag
  }
}

double solve_backtracking(Item *items, int n, int capacity)
{
  const char *method_name = "回溯法";
  if (n > MAX_N_FOR_BACKTRACKING)
  {
    printf("\n--- %s ---\n", method_name);
    printf("跳过执行：N=%d 个物品 (对于回溯法来说数量过大，上限 %d)。\n", n, MAX_N_FOR_BACKTRACKING);
    printf("-------------------------------------\n");
    return TIME_SKIPPED;
  }
  if (n > 30 && MAX_N_FOR_BACKTRACKING > 30)
  {
    printf("\n--- %s (警告) ---\n", method_name);
    printf("警告：N=%d 对于回溯法可能非常大，即使在上限(%d)内，也可能耗时极长。\n", n, MAX_N_FOR_BACKTRACKING);
    printf("-------------------------------------\n");
  }

  g_max_value_backtracking = 0;
  g_best_weight_backtracking = 0;
  g_best_item_count_backtracking = 0;
  if (g_best_selection_backtracking != NULL)
  {
    free(g_best_selection_backtracking);
    g_best_selection_backtracking = NULL;
  }
  g_best_selection_backtracking = (int *)malloc(n * sizeof(int));
  if (!g_best_selection_backtracking)
  {
    perror("为回溯法最佳选择分配内存失败");
    print_solution_details(method_name, items, n, NULL, -1, 0, 0);
    return TIME_ERROR;
  }

  bool *current_selection_flags = (bool *)malloc(n * sizeof(bool));
  if (!current_selection_flags)
  {
    perror("为回溯法当前选择标记分配内存失败");
    print_solution_details(method_name, items, n, NULL, -1, 0, 0);
    return TIME_ERROR;
  }
  for (int i = 0; i < n; ++i)
    current_selection_flags[i] = false;

  clock_t start_time = clock();
  knapsack_backtracking_recursive(items, n, capacity, 0, 0, 0, current_selection_flags);
  clock_t end_time = clock();
  double time_taken = ((double)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0;

  print_solution_details(method_name, items, n, g_best_selection_backtracking, g_best_item_count_backtracking, g_max_value_backtracking, g_best_weight_backtracking);

  free(current_selection_flags);
  // g_best_selection_backtracking将在main函数末尾或下次调用solve_backtracking时被释放
  return time_taken;
}

// --- 数据生成 ---
Item *generate_items(int n)
{
  Item *items = (Item *)malloc(n * sizeof(Item));
  if (!items)
  {
    perror("物品内存分配失败");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < n; i++)
  {
    items[i].id = i + 1; // Assign unique ID
    items[i].weight = rand() % 100 + 1;
    items[i].value = rand() % 901 + 100;
    items[i].ratio = 0;
  }
  return items;
}

// --- 为N=1000的情况输出物品统计信息到控制台并生成CSV文件 ---
void output_item_statistics_for_n1000(Item *items, int n, int capacity)
{
  if (n != 1000)
    return;

  char filename[100];
  sprintf(filename, "knapsack_items_N%d_C%d_details.csv", n, capacity);
  FILE *csv_file = fopen(filename, "w");
  if (!csv_file)
  {
    perror("无法创建N=1000物品详情的CSV文件");
    return;
  }

  printf("\n--- N=%d, 容量=%d 时的物品统计信息 ---\n", n, capacity);
  printf("表1. 容量为 %d 的 0-1 背包物品统计信息示例 (N=%d)\n", capacity, n);
  printf("--------------------------------------------------\n");
  printf("%-10s %-10s %-10s\n", "物品编号", "物品重量", "物品价值");
  printf("--------------------------------------------------\n");
  fprintf(csv_file, "物品编号,物品重量,物品价值\n");

  int print_to_console_limit = 5;
  if (n <= print_to_console_limit * 2)
    print_to_console_limit = n;

  for (int i = 0; i < n; i++)
  {
    if (n <= print_to_console_limit * 2)
    {
      printf("%-10d %-10d %-10d\n", items[i].id, items[i].weight, items[i].value);
    }
    else
    {
      if (i < print_to_console_limit || i >= n - print_to_console_limit)
      {
        printf("%-10d %-10d %-10d\n", items[i].id, items[i].weight, items[i].value);
      }
      else if (i == print_to_console_limit)
      {
        printf("......      ......      ......\n");
      }
    }
    fprintf(csv_file, "%d,%d,%d\n", items[i].id, items[i].weight, items[i].value);
  }
  printf("--------------------------------------------------\n");
  printf("N=%d 的物品详细数据已写入到文件: %s\n", n, filename);
  printf("--- N=%d 物品统计信息结束 ---\n\n", n);
  fclose(csv_file);
}

// Helper to add timing data to the global array
void add_timing_entry(int n, int c, double times_local[4])
{
  if (timing_data_count >= timing_data_capacity)
  {
    // Basic dynamic resizing, could be more sophisticated
    timing_data_capacity = (timing_data_capacity == 0) ? 10 : timing_data_capacity * 2;
    TimingInfo *new_data = realloc(all_timing_data, timing_data_capacity * sizeof(TimingInfo));
    if (!new_data)
    {
      perror("Failed to reallocate memory for timing data");
      // Not freeing all_timing_data here as it might still be valid or main will try to free it
      exit(EXIT_FAILURE);
    }
    all_timing_data = new_data;
  }
  all_timing_data[timing_data_count].n = n;
  all_timing_data[timing_data_count].c = c;
  memcpy(all_timing_data[timing_data_count].times, times_local, 4 * sizeof(double));
  timing_data_count++;
}

// --- 主函数 ---
int main()
{
  srand(time(NULL));

  double current_run_times[4]; // 0: BF, 1: DP, 2: Greedy, 3: BT
  const char *algo_names[] = {"蛮力法", "动态规划", "贪心法", "回溯法"};

  // --- 示例测试用例 (N=30) ---
  int n_example = 30;
  int capacity_example = 1000;
  printf("\n\n##########################################\n");
  printf("开始测试: N = %d, 容量 = %d (示例)\n", n_example, capacity_example);
  printf("##########################################\n");
  Item *items_example = generate_items(n_example);
  current_run_times[0] = solve_bruteforce(items_example, n_example, capacity_example);
  current_run_times[1] = solve_dp(items_example, n_example, capacity_example);
  current_run_times[2] = solve_greedy(items_example, n_example, capacity_example);
  current_run_times[3] = solve_backtracking(items_example, n_example, capacity_example);
  add_timing_entry(n_example, capacity_example, current_run_times);
  free(items_example);
  printf("\n--- (N=%d, C=%d) 执行时间摘要 ---\n", n_example, capacity_example);
  for (int k = 0; k < 4; ++k)
  {
    printf("%-12s 执行时间: ", algo_names[k]);
    if (current_run_times[k] == TIME_SKIPPED)
      printf("SKIPPED\n");
    else if (current_run_times[k] == TIME_ERROR)
      printf("ERROR\n");
    else
      printf("%.2f 毫秒\n", current_run_times[k]);
  }
  printf("------------------------------------------\n");
  printf("--- 示例测试用例结束 ---\n");

  // --- 特定 N 和 Capacity 的执行时间测试 ---
  int n_specific = 25;
  int capacity_specific = 800;
  printf("\n\n##########################################\n");
  printf("开始测试: N = %d, 容量 = %d (特定测试)\n", n_specific, capacity_specific);
  printf("##########################################\n");
  Item *items_specific_test = generate_items(n_specific);
  current_run_times[0] = solve_bruteforce(items_specific_test, n_specific, capacity_specific);
  current_run_times[1] = solve_dp(items_specific_test, n_specific, capacity_specific);
  current_run_times[2] = solve_greedy(items_specific_test, n_specific, capacity_specific);
  current_run_times[3] = solve_backtracking(items_specific_test, n_specific, capacity_specific);
  add_timing_entry(n_specific, capacity_specific, current_run_times);
  free(items_specific_test);
  printf("\n--- (N=%d, C=%d) 执行时间摘要 ---\n", n_specific, capacity_specific);
  for (int k = 0; k < 4; ++k)
  {
    printf("%-12s 执行时间: ", algo_names[k]);
    if (current_run_times[k] == TIME_SKIPPED)
      printf("SKIPPED\n");
    else if (current_run_times[k] == TIME_ERROR)
      printf("ERROR\n");
    else
      printf("%.2f 毫秒\n", current_run_times[k]);
  }
  printf("------------------------------------------\n");
  printf("--- 特定 N 和 Capacity 的执行时间测试结束 ---\n");

  // --- 指定的输入规模 ---
  int N_values[] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000, 40000, 80000, 160000, 320000};
  int C_values[] = {10000, 100000, 1000000};
  int num_N_values = sizeof(N_values) / sizeof(N_values[0]);
  int num_C_values = sizeof(C_values) / sizeof(C_values[0]);

  for (int i = 0; i < num_N_values; i++)
  {
    int n_loop = N_values[i];
    for (int j = 0; j < num_C_values; j++)
    {
      int capacity_loop = C_values[j];
      printf("\n\n##########################################\n");
      printf("开始测试: N = %d, 容量 = %d (循环 %d/%d, %d/%d)\n", n_loop, capacity_loop, i + 1, num_N_values, j + 1, num_C_values);
      printf("##########################################\n");
      Item *items_generated = generate_items(n_loop);
      if (n_loop == 1000 && j == 0) // j==0 确保只对 N=1000 的第一个C值执行
      {
        output_item_statistics_for_n1000(items_generated, n_loop, capacity_loop);
      }
      current_run_times[0] = solve_bruteforce(items_generated, n_loop, capacity_loop);
      current_run_times[1] = solve_dp(items_generated, n_loop, capacity_loop);
      current_run_times[2] = solve_greedy(items_generated, n_loop, capacity_loop);
      current_run_times[3] = solve_backtracking(items_generated, n_loop, capacity_loop);
      add_timing_entry(n_loop, capacity_loop, current_run_times);
      free(items_generated);

      printf("\n--- (N=%d, C=%d) 执行时间摘要 ---\n", n_loop, capacity_loop);
      for (int k = 0; k < 4; ++k)
      {
        printf("%-12s 执行时间: ", algo_names[k]);
        if (current_run_times[k] == TIME_SKIPPED)
          printf("SKIPPED\n");
        else if (current_run_times[k] == TIME_ERROR)
          printf("ERROR\n");
        else
          printf("%.2f 毫秒\n", current_run_times[k]);
      }
      printf("------------------------------------------\n");
    }
  }

  // --- 打印所有时间的最终表格 ---
  printf("\n\n#################################################################################\n");
  printf("###                         所有测试执行时间总表 (毫秒)                       ###\n");
  printf("#################################################################################\n");
  printf("| %-8s | %-10s | %-18s | %-18s | %-15s | %-18s |\n", "N", "Capacity", algo_names[0], algo_names[1], algo_names[2], algo_names[3]);
  printf("|----------|------------|--------------------|--------------------|-----------------|--------------------|\n");

  char time_str_bf[20], time_str_dp[20], time_str_greedy[20], time_str_bt[20];

  for (int i = 0; i < timing_data_count; ++i)
  {
    TimingInfo current_data = all_timing_data[i];

    if (current_data.times[0] == TIME_SKIPPED)
      sprintf(time_str_bf, "%-18s", "SKIPPED");
    else if (current_data.times[0] == TIME_ERROR)
      sprintf(time_str_bf, "%-18s", "ERROR");
    else
      sprintf(time_str_bf, "%-18.2f", current_data.times[0]);

    if (current_data.times[1] == TIME_SKIPPED)
      sprintf(time_str_dp, "%-18s", "SKIPPED");
    else if (current_data.times[1] == TIME_ERROR)
      sprintf(time_str_dp, "%-18s", "ERROR");
    else
      sprintf(time_str_dp, "%-18.2f", current_data.times[1]);

    if (current_data.times[2] == TIME_SKIPPED)
      sprintf(time_str_greedy, "%-15s", "SKIPPED");
    else if (current_data.times[2] == TIME_ERROR)
      sprintf(time_str_greedy, "%-15s", "ERROR");
    else
      sprintf(time_str_greedy, "%-15.2f", current_data.times[2]);

    if (current_data.times[3] == TIME_SKIPPED)
      sprintf(time_str_bt, "%-18s", "SKIPPED");
    else if (current_data.times[3] == TIME_ERROR)
      sprintf(time_str_bt, "%-18s", "ERROR");
    else
      sprintf(time_str_bt, "%-18.2f", current_data.times[3]);

    printf("| %-8d | %-10d | %s | %s | %s | %s |\n",
           current_data.n, current_data.c,
           time_str_bf, time_str_dp, time_str_greedy, time_str_bt);
  }
  printf("#################################################################################\n");

  // 释放全局变量 (如果它们在最后一次调用后可能仍然分配着)
  if (g_best_selection_bruteforce)
  {
    free(g_best_selection_bruteforce);
    g_best_selection_bruteforce = NULL;
  }
  if (g_best_selection_backtracking)
  {
    free(g_best_selection_backtracking);
    g_best_selection_backtracking = NULL;
  }
  if (all_timing_data)
  {
    free(all_timing_data);
    all_timing_data = NULL;
  }

  printf("\n所有测试完成。\n");
  return 0;
}