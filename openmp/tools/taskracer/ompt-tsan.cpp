#include <iostream>
#include <cassert>
#include "omp-tools.h"
#include "dlfcn.h"

enum NodeType{
  ROOT,
  FINISH,
  ASYNC,
  FUTURE,
  STEP,
  TASKWAIT
};

typedef struct TreeNode {
 public:
  // int index;
  int corresponding_task_id;
  int corresponding_step_index;
  NodeType this_node_type;
  int depth;
  int number_of_child;
  int is_parent_nth_child;
  int preceeding_taskwait;
  TreeNode *parent;
  TreeNode *children_list_head;
  TreeNode *children_list_tail;
  TreeNode *next_sibling;
  TreeNode *current_finish_node;
  TreeNode() = default;
  TreeNode(int task_id, int step_index, NodeType type, int depth, int nth_child,
           int preceeding_taskwait, TreeNode *parent);
} TreeNode;


typedef struct finish_t{
  TreeNode* node_in_dpst;
  finish_t* parent;
} finish_t;

typedef struct task_t{
  int id;
  int parent_id;
  TreeNode* node_in_dpst;
  finish_t* belong_to_finish;
  finish_t* current_finish;
} task_t;


extern "C" {
void __attribute__((weak)) __tsan_print();

TreeNode* __attribute__((weak))  insert_tree_node(NodeType nodeType, TreeNode *parent);

TreeNode* __attribute__((weak))  insert_leaf(TreeNode *task_node);

void __attribute__((weak))  DPSTinfo();

void __attribute__((weak))  putNodeInCurThread(TreeNode* node);

void __attribute__((weak))  set_ompt_ready(bool b);
} // extern "C"


#define SET_OPTIONAL_CALLBACK_T(event, type, result, level)                    \
  do {                                                                         \
    ompt_callback_##type##_t ta_##event = &ompt_ta_##event;                    \
    result = ompt_set_callback(ompt_callback_##event,                          \
                               (ompt_callback_t)ta_##event);                   \
    if (result < level)                                                        \
      printf("Registered callback '" #event "' is not supported at " #level    \
             " (%i)\n",                                                        \
             result);                                                          \
  } while (0)

#define SET_CALLBACK_T(event, type)                                            \
  do {                                                                         \
    int res;                                                                   \
    SET_OPTIONAL_CALLBACK_T(event, type, res, ompt_set_always);                \
  } while (0)

#define SET_CALLBACK(event) SET_CALLBACK_T(event, event)

ompt_get_task_info_t ompt_get_task_info;
ompt_set_callback_t ompt_set_callback;


static std::atomic<int> task_id_counter(1);

static void ompt_ta_parallel_begin
(
  ompt_data_t *encountering_task_data,
  const ompt_frame_t *encountering_task_frame,
  ompt_data_t *parallel_data,
  unsigned int requested_parallelism,
  int flags,
  const void *codeptr_ra
)
{
  assert(encountering_task_data->ptr != nullptr);
  // insert a FINISH node because of the implicit barrier
  task_t* current_task = (task_t*) encountering_task_data->ptr;
  TreeNode* current_task_node = current_task->node_in_dpst;

  // 1. Update DPST
  TreeNode* new_finish_node;
  if(current_task->current_finish == nullptr){
    new_finish_node = insert_tree_node(FINISH,current_task_node);
  }
  else{
    new_finish_node = insert_tree_node(FINISH,current_task->current_finish->node_in_dpst);
  }
  insert_leaf(new_finish_node);
  insert_leaf(current_task_node);

  // 2. Set current task's current_finish to this finish
  finish_t* finish = (finish_t*) malloc(sizeof(finish_t));
  finish->node_in_dpst = new_finish_node;

  if(current_task->current_finish == nullptr){
    finish->parent = nullptr;
  }
  else{
    finish->parent = current_task->current_finish;
  }

  current_task->current_finish = finish;
  current_task_node->current_finish_node = new_finish_node;


  parallel_data->ptr = encountering_task_data->ptr;
}


static void ompt_ta_parallel_end
(
  ompt_data_t *parallel_data,
  ompt_data_t *encountering_task_data,
  int flags,
  const void *codeptr_ra
)
{
  assert(encountering_task_data->ptr != nullptr);

  // set current_task->current_finish to either nullptr or another finish (nested finish)
  task_t* current_task = (task_t*) encountering_task_data->ptr;
  finish_t* the_finish = current_task->current_finish;

  if(the_finish->parent == nullptr){
    current_task->current_finish = nullptr;
    current_task->node_in_dpst->current_finish_node = nullptr;
  }
  else{
    current_task->current_finish = the_finish->parent;
    current_task->node_in_dpst->current_finish_node = the_finish->parent->node_in_dpst;
  }
}


static void ompt_ta_implicit_task(
  ompt_scope_endpoint_t endpoint,
  ompt_data_t *parallel_data,
  ompt_data_t *task_data,
  unsigned int actual_parallelism,
  unsigned int index,
  int flags
)
{
  if(flags & ompt_task_initial){
    if(endpoint == ompt_scope_begin){
      printf("OMPT! initial task begins, should only appear once !! \n");

      // DPST operation
      TreeNode* root = insert_tree_node(ROOT,nullptr);
      insert_leaf(root);

      task_t* main_ti = (task_t*) malloc(sizeof(task_t));
      main_ti->belong_to_finish = nullptr;
      main_ti->id = 0;
      main_ti->parent_id = -1;
      main_ti->node_in_dpst = root;

      finish_t* main_finish_placeholder = (finish_t*) malloc(sizeof(finish_t));
      main_finish_placeholder->node_in_dpst = root;
      main_finish_placeholder->parent = nullptr;
      main_ti->current_finish = main_finish_placeholder;
      main_ti->node_in_dpst->current_finish_node = main_finish_placeholder->node_in_dpst;

      task_data->ptr = (void*) main_ti;

      set_ompt_ready(true);
    }
  }
  else{
    if(endpoint == ompt_scope_begin){
      assert(parallel_data->ptr != nullptr);

      // A. Get encountering_task information
      task_t* current_task = (task_t*) parallel_data->ptr;
      TreeNode* current_task_node = current_task->node_in_dpst;

      // B. DPST operation
        TreeNode* new_task_node;
        TreeNode* parent_node;

        if(current_task->current_finish != nullptr){
          // 1. if current task's current_finish is not nullptr, parent_node should be that finish's node
          parent_node = current_task->current_finish->node_in_dpst;
        }
        else{
          // 2. if current task's current_finish is nullptr, parent_node should be current task's node
          parent_node = current_task_node;
        }

        new_task_node = insert_tree_node(ASYNC, parent_node);

        insert_leaf(new_task_node->parent);
        insert_leaf(new_task_node);
        new_task_node->corresponding_task_id = task_id_counter;


      // C. Update task data for new task
        task_t* ti = (task_t*) malloc(sizeof(task_t));
        ti->id = task_id_counter;
        ti->node_in_dpst = new_task_node;

        // set new task's belong_to_finish
        if(current_task->current_finish != nullptr){
          ti->belong_to_finish = current_task->current_finish;
        }
        else{
          ti->belong_to_finish = current_task->belong_to_finish;
        }
        
        task_data->ptr = (void*) ti;

      // D. Update global variable
        task_id_counter ++;
    }
  }

}


static void ompt_ta_sync_region(
  ompt_sync_region_t kind,
  ompt_scope_endpoint_t endpoint,
  ompt_data_t *parallel_data,
  ompt_data_t *task_data,
  const void *codeptr_ra)
{
  if(kind == ompt_sync_region_taskgroup && endpoint == ompt_scope_begin){
    assert(task_data->ptr != nullptr);

    task_t* current_task = (task_t*) task_data->ptr;
    TreeNode* current_task_node = current_task->node_in_dpst;

    // 1. Update DPST
    TreeNode* new_finish_node;
    if(current_task->current_finish == nullptr){
      new_finish_node = insert_tree_node(FINISH,current_task_node);
    }
    else{
      new_finish_node = insert_tree_node(FINISH,current_task->current_finish->node_in_dpst);
    }
    insert_leaf(new_finish_node);
    insert_leaf(current_task_node);

    // 2. Set current task's current_finish to this finish
    finish_t* new_finish = (finish_t*) malloc(sizeof(finish_t));
    new_finish->node_in_dpst = new_finish_node;

    if(current_task->current_finish == nullptr){
      new_finish->parent = nullptr;
    }
    else{
      new_finish->parent = current_task->current_finish;
    }

    current_task->current_finish = new_finish;
    current_task_node->current_finish_node = new_finish->node_in_dpst;

  }
  else if (kind == ompt_sync_region_taskgroup && endpoint == ompt_scope_end )
  {
    assert(task_data->ptr != nullptr);

    // set current_task->current_finish to either nullptr or another finish (nested finish)
    task_t* current_task = (task_t*) task_data->ptr;
    finish_t* finish = current_task->current_finish;

    if(finish->parent == nullptr){
      current_task->current_finish = nullptr;
      current_task->node_in_dpst->current_finish_node = nullptr;
    }
    else{
      current_task->current_finish = finish->parent;
      current_task->node_in_dpst->current_finish_node = finish->parent->node_in_dpst;
    }
    
  }
  else if(kind == ompt_sync_region_taskwait && endpoint == ompt_scope_begin){
    assert(task_data->ptr != nullptr);
    task_t* current_task = (task_t*) task_data->ptr;

    // insert a single node (type STEP), mark this as a taskwait step
    TreeNode* new_taskwait_node;
    if(current_task->current_finish == nullptr){
      new_taskwait_node = insert_tree_node(TASKWAIT, current_task->node_in_dpst);
      new_taskwait_node->preceeding_taskwait = new_taskwait_node->is_parent_nth_child;
      insert_leaf(current_task->node_in_dpst);
    }
    else{
      new_taskwait_node = insert_tree_node(TASKWAIT, current_task->current_finish->node_in_dpst);
      new_taskwait_node->preceeding_taskwait = new_taskwait_node->is_parent_nth_child;
      insert_leaf(current_task->current_finish->node_in_dpst);
    }

  }
  
}


static void ompt_ta_task_create(ompt_data_t *encountering_task_data,
                                const ompt_frame_t *encountering_task_frame,
                                ompt_data_t *new_task_data, int flags,
                                int has_dependences, const void *codeptr_ra) 
{
  assert(encountering_task_data->ptr != nullptr);

  // A. Get encountering_task information
    task_t* current_task = (task_t*) encountering_task_data->ptr;
    TreeNode* current_task_node = current_task->node_in_dpst;

  // B. DPST operation
    TreeNode* new_task_node;
    TreeNode* parent_node;

    if(current_task->current_finish != nullptr){
      // 1. if current task's current_finish is not nullptr, parent_node should be that finish's node
      parent_node = current_task->current_finish->node_in_dpst;
    }
    else{
      // 2. if current task's current_finish is nullptr, parent_node should be current task's node
      parent_node = current_task_node;
    }

    new_task_node = insert_tree_node(ASYNC, parent_node);

    insert_leaf(new_task_node->parent);
    insert_leaf(new_task_node);
    new_task_node->corresponding_task_id = task_id_counter;

  // C. Update task data
    task_t* ti = (task_t*) malloc(sizeof(task_t));
    ti->id = task_id_counter;
    ti->node_in_dpst = new_task_node;

    // set new task's belong_to_finish
    if(current_task->current_finish != nullptr){
      ti->belong_to_finish = current_task->current_finish;
    }
    else{
      ti->belong_to_finish = current_task->belong_to_finish;
    }
    
    new_task_data->ptr = (void*) ti;

  // D. Update global variable
    task_id_counter ++;
}



static void ompt_ta_task_schedule(
  ompt_data_t *prior_task_data,
  ompt_task_status_t prior_task_status,
  ompt_data_t *next_task_data
){
  assert(next_task_data->ptr);

  task_t* next_task = (task_t*) next_task_data->ptr;
  TreeNode* next_task_node = next_task->node_in_dpst;
  // printf("OMPT! task_schedule, put task node in current thread \n");
  putNodeInCurThread(next_task_node);
}


static int ompt_tsan_initialize(ompt_function_lookup_t lookup, int device_num,
                                ompt_data_t *tool_data) {
  // __tsan_print();
  ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
  if (ompt_set_callback == NULL) {
    std::cerr << "Could not set callback, exiting..." << std::endl;
    std::exit(1);
  }

  ompt_get_task_info = (ompt_get_task_info_t) lookup("ompt_get_task_info");
  if(ompt_get_task_info == NULL){
    std::cerr << "Could not get task info, exiting..." << std::endl;
    std::exit(1);
  }

  SET_CALLBACK(task_create);
  SET_CALLBACK(parallel_begin);
  SET_CALLBACK(implicit_task);
  SET_CALLBACK(sync_region);
  SET_CALLBACK(parallel_end);
  SET_CALLBACK(task_schedule);

  return 1; // success
}

static void ompt_tsan_finalize(ompt_data_t *tool_data) {
  set_ompt_ready(false);
  DPSTinfo();
}

static bool scan_tsan_runtime() {
  void *func_ptr = dlsym(RTLD_DEFAULT, "__tsan_read1");
  return func_ptr != nullptr;
}

extern "C" ompt_start_tool_result_t *
ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
  static ompt_start_tool_result_t ompt_start_tool_result = {
      &ompt_tsan_initialize, &ompt_tsan_finalize, {0}};
  if (!scan_tsan_runtime()) {
    std::cout << "Taskracer does not detect TSAN runtime, stopping operation" << std::endl;
    return nullptr;
  }
  std::cout << "Taskracer detects TSAN runtime, carrying out race detection using DPST" << std::endl;
  return &ompt_start_tool_result;
}