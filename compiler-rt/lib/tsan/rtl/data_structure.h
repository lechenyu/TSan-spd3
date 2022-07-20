#ifndef   DATA_STRUCTURE_H
#define   DATA_STRUCTURE_H

extern "C" {
enum node_type{
    ROOT,
    FINISH,
    ASYNC,
    FUTURE,
    STEP,
    TASKWAIT
};

typedef struct tree_node{
    int index;
    int corresponding_task_id;
    enum node_type this_node_type;
    int depth;
    int number_of_child;
    int is_parent_nth_child;
    int preceeding_taskwait;
    struct tree_node *parent;
    struct tree_node *children_list_head;
    struct tree_node *children_list_tail;
    struct tree_node *next_sibling;
    struct tree_node *current_finish_node;
    tree_node();
} tree_node;


typedef struct dpst{
    struct tree_node *root;
    struct tree_node *current_step_node;
    int height;
} dpst;

extern struct dpst DPST;

typedef struct finish_t{
    tree_node* node_in_dpst;
    finish_t* parent;
} finish_t;

typedef struct task_t{
  int id;
  int parent_id;
  tree_node* node_in_dpst;
  finish_t* belong_to_finish;
  finish_t* current_finish;
} task_t;

}

#endif