#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_allocator_internal.h"
#include "sanitizer_common/sanitizer_vector.h"
#include "tsan_rtl.h"
#include "concurrency_vector.h"

namespace __tsan{

extern ConcurrencyVector step_nodes;

// dpst data structure
static DPST dpst;

static const char* node_desc[] = {"Root","Finish", "Async", "Future", "Step", "TaskWait"};

// static int node_index_base = 0;
// atomic_uint32_t *atomic_node_index = reinterpret_cast<atomic_uint32_t *>(&node_index_base);

// static u32 step_index_base = 1;
// atomic_uint32_t *atomic_step_index = reinterpret_cast<atomic_uint32_t *>(&step_index_base);

extern "C" {
INTERFACE_ATTRIBUTE
void __tsan_set_task_in_tls(TreeNode* node){
    // Printf("new task node index is %d \n", node->index);
    ThreadState* current_thread = cur_thread();
    current_thread->current_task_node = node;
}

/**
 * @brief  Create a new tree node
 * @note   
 * @retval The newly created tree node
 */
static TreeNode *newtreeNode()
{
    // Allocate memory for new node
    // tree_node* node = new tree_node();
    TreeNode *node = (TreeNode *) InternalAlloc(sizeof(TreeNode));
    node->children_list_head = nullptr;
    node->children_list_tail = nullptr;
    node->next_sibling = nullptr;
    node->current_finish_node = nullptr;

    node->corresponding_task_id = -2;
    node->number_of_child = 0;
    node->is_parent_nth_child = 0;
    node->preceeding_taskwait = -1;

    // u32 node_index = atomic_load(atomic_node_index,memory_order_acquire);
    // node->index = node_index;
    // atomic_store(atomic_node_index,node_index+1,memory_order_consume);

    node->corresponding_step_index = 0;

    return node;
}


/**
 * @brief  Insert a tree node (not a step node) under parent
 * @note   
 * @param  nodeType: root,async,future or finish
 * @param  *parent: parent node, the inserted node will become a child of parent
 * @retval the newly inserted node
 */
INTERFACE_ATTRIBUTE
TreeNode *__tsan_insert_tree_node(NodeType nodeType, TreeNode *parent){
    TreeNode *node = newtreeNode();   
    node->this_node_type = nodeType;
    
    if(nodeType == ROOT){ 
        node->depth = 0;
        node->parent = nullptr;
        dpst.root = node;
    }
    else{
        // each task corresponds to an async or a future tree node
        // assert(parent);
        node->parent = parent;
        node->depth = node->parent->depth + 1;
        node->is_parent_nth_child = parent->number_of_child;
        if(nodeType == FINISH){
            node->corresponding_task_id = parent->corresponding_task_id;
        }
        parent->number_of_child += 1;

        if(node->parent->children_list_head == nullptr){
            node->parent->children_list_head = node;
            node->parent->children_list_tail = node;
        }
        else{
            node->parent->children_list_tail->next_sibling = node;
            node->preceeding_taskwait = node->parent->children_list_tail->preceeding_taskwait;
            node->parent->children_list_tail = node;
        }   
    }

    return node;
}


/**
 * @brief  Insert a new leaf(step) node to the task_node
 * @note   
 * @param  *task_node: the node that will have a new leaf
 * @retval the newly inserted leaf(step) node
 */
INTERFACE_ATTRIBUTE
TreeNode *__tsan_insert_leaf(TreeNode *task_node) {
  ThreadState *thr = cur_thread();
  int preceeding_taskwait =
      (task_node->children_list_head)
          ? task_node->children_list_tail->preceeding_taskwait
          : -1;
  TreeNode &n = step_nodes.EmplaceBack(
      task_node->corresponding_task_id, STEP, task_node->depth + 1,
      task_node->number_of_child, preceeding_taskwait, thr->fast_state.sid(), thr->fast_state.epoch(), task_node);
  TreeNode *new_step = &n;
  task_node->number_of_child += 1;

  if (task_node->children_list_head == nullptr) {
    task_node->children_list_head = new_step;
    task_node->children_list_tail = new_step;
  } else {
    task_node->children_list_tail->next_sibling = new_step;
    task_node->children_list_tail = new_step;
  }

  // u32 step_index = atomic_load(atomic_step_index, memory_order_acquire);
  // atomic_store(atomic_step_index,step_index+1,memory_order_consume);

  // new_step->corresponding_step_index = step_index;
  // step_nodes[step_index] = new_step;
  // Printf("insert into %d\n", step_index);

  return new_step;
}

static void print_node(TreeNode *t) {
  constexpr uptr mask = 0xFFFFUL;
  Printf("[%s_%lu]", node_desc[t->this_node_type],
         t->corresponding_step_index ? t->corresponding_step_index : (uptr)t & mask);
  TreeNode *p = t->parent;
  if (p) {
    Printf("(%s_%lu[%u])", node_desc[p->this_node_type],
           p->corresponding_step_index ? p->corresponding_step_index : (uptr)p & mask,
           t->is_parent_nth_child);
  }
}

static void get_dpst_statistics(u32 *counts, int &max_depth, bool print_dpst) {
  if (!dpst.root) {
    return;
  }
  if (print_dpst) {
    Printf("DPST Structure: \n");
  }
  Vector<TreeNode *> v1, v2;
  Vector<TreeNode *> *curr = &v1, *next = &v2;
  curr->PushBack(dpst.root);
  u32 level = 0;
  while (curr->Size()) {
    if (print_dpst) {
      Printf(" Level %u: ", level);
    }
    for (TreeNode *t : *curr) {
      if (print_dpst) {
        print_node(t);
        Printf(" ");
      }
      max_depth = (max_depth < t->depth) ? t->depth : max_depth;
      counts[t->this_node_type]++;
      for (TreeNode *c = t->children_list_head; c != nullptr;
           c = c->next_sibling) {
        next->PushBack(c);
      }
    }
    level++;
    Swap(curr, next);
    next->Clear();
    if (print_dpst) {
      Printf("\n");
    }
  }
}

INTERFACE_ATTRIBUTE
void __tsan_print_dpst_info(bool print_dpst) {
  Printf("=============================================================\n");
  u32 node_counts[NODE_TYPE_END]{};
  int max_depth{};
  u32 num_nodes{};
  get_dpst_statistics(node_counts, max_depth, print_dpst);
  Printf("\nDPST Statistics:\n");
  for (u32 i = 0; i < NODE_TYPE_END; i++) {
    Printf("%s nodes: %u\n", node_desc[i], node_counts[i]);
    num_nodes += node_counts[i];
  }
  Printf("Total number of nodes: %u\n", num_nodes);
  Printf("Max depth: %d\n", max_depth);
  Printf("=============================================================\n");
}

} // extern C
} // namespace __tsan