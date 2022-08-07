#ifndef   DATA_STRUCTURE_H
#define   DATA_STRUCTURE_H

namespace __tsan {
enum NodeType {
  ROOT,
  FINISH,
  ASYNC,
  FUTURE,
  STEP,
  TASKWAIT
};

class TreeNode {
 public:
  int index;
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
};


class dpst{
 public:
  TreeNode *root;
  TreeNode *current_step_node;
  int height;
  dpst() {
    this->root = nullptr;
    this->height = 0;
    this->current_step_node = nullptr;
  }
};

// extern struct dpst DPST;

// typedef struct finish_t{
//     tree_node* node_in_dpst;
//     finish_t* parent;
// } finish_t;

// typedef struct task_t{
//   int id;
//   int parent_id;
//   tree_node* node_in_dpst;
//   finish_t* belong_to_finish;
//   finish_t* current_finish;
// } task_t;

} // namespace __tsan
#endif