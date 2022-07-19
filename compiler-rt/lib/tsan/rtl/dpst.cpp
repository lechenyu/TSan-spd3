#include "data_structure.h"

#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_stacktrace.h"

using namespace __tsan;

// dpst data structure
struct dpst DPST;

char node_char[6] = {'R','F','A','f','S','W'};
static int node_index = 0;

extern "C" {
tree_node::tree_node(){

}

/**
 * @brief  Create a new tree node
 * @note   
 * @retval The newly created tree node
 */
tree_node* newtreeNode()
{
    // Allocate memory for new node
    tree_node* node = new tree_node();
    node->children_list_head = nullptr;
    node->children_list_tail = nullptr;
    node->next_sibling = nullptr;
    node->corresponding_task_id = -2;
    node->number_of_child = 0;
    node->is_parent_nth_child = 0;
    node->preceeding_taskwait = -1;

    node->index = node_index;
    node_index ++;
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
tree_node* insert_tree_node(enum node_type nodeType, tree_node *parent){
    tree_node *node = newtreeNode();   
    node->this_node_type = nodeType;
    
    if(nodeType == ROOT){ 
        node->depth = 0;
        node->parent = nullptr;
        DPST.root = node;
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

    if(node->depth > DPST.height){
        DPST.height = node->depth;
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
tree_node* insert_leaf(tree_node *task_node){
    // HASSERT(task_node);
    tree_node *new_step = newtreeNode();   
    new_step->this_node_type = STEP;
    new_step->parent = task_node;
    new_step->depth = task_node->depth + 1;
    new_step->is_parent_nth_child = task_node->number_of_child;
    new_step->corresponding_task_id = task_node->corresponding_task_id;

    task_node->number_of_child += 1;
    
    if(task_node->children_list_head == nullptr){
        task_node->children_list_head = new_step;
        task_node->children_list_tail = new_step;
    }
    else{
        task_node->children_list_tail->next_sibling = new_step;
        new_step->preceeding_taskwait = task_node->children_list_tail->preceeding_taskwait;
        task_node->children_list_tail = new_step;
    }

    if(new_step->depth > DPST.height){
        DPST.height = new_step->depth;
    }
    return new_step;
}


/**
 * @brief  check if node1 precedes node2 in dpst
 * @param  node1: previous step node
 * @param  node2: current step node
 * @retval true if node1 precdes node2 by tree edges 
 */
INTERFACE_ATTRIBUTE
bool precede_dpst(tree_node* node1, tree_node* node2){
  bool node1_precede_node2 = true;
  bool node1_not_precede_node2 = false;

    if(node1->parent->index == node2->parent->index){
        // node1 and node2 are step nodes with same parent
        if(node1->is_parent_nth_child <= node2->is_parent_nth_child){
          return node1_precede_node2;
        }
        else{
          return node1_not_precede_node2;
        }
    }
    
    // need to guarantee prev_node is to the left of current_node
    tree_node* node1_last_node;
    tree_node* node2_last_node;

    while (node1->depth != node2->depth)
    {
        node1_last_node = node1;
        node2_last_node = node2;
        if (node1->depth > node2->depth)
        {
            node1 = node1->parent;
        }
        else{
            node2 = node2->parent;
        }
    }

    while(node1->index != node2->index){
        node1_last_node = node1;
        node2_last_node = node2;
        node1 = node1->parent;
        node2 = node2->parent;
    }; // end

    if(node1_last_node->is_parent_nth_child < node2_last_node->is_parent_nth_child){
        // node1 is to the left of node 2
        if(
             (node1_last_node->this_node_type == ASYNC && node2_last_node->preceeding_taskwait < 0)
          || (node1_last_node->this_node_type == ASYNC && node2_last_node->preceeding_taskwait < node1_last_node->is_parent_nth_child)
        ){
          return node1_not_precede_node2;
        }
        else{
            return node1_precede_node2;
        }
    }

    // node 1 is to the right of node 2
    // return false because "node1 doesn't precede node2 by DPST"
    return node1_not_precede_node2;
}


int get_dpst_height(){
    return DPST.height;
}

INTERFACE_ATTRIBUTE
void printDPST(){
    tree_node *node_array[100] = {nullptr};
    tree_node *tmp_array[100] = {nullptr};
    node_array[0] = DPST.root;
    int depth = 0;

    while (node_array[0] != nullptr)
    {
        Printf("depth %d:   ",depth);
        int tmp_index = 0;
        int i = 0;
        while (i < 100)
        {
            tree_node *node = node_array[i];
            if(node == nullptr){
                //Printf("   ");
            }
            else{
                Printf("%c (i:%d) ",node_char[node->this_node_type],node->index);
                if(node->parent != nullptr){
                    Printf("(p:%d)    ",node->parent->index);
                }
                tree_node *child = node->children_list_head;
                while (child != nullptr)
                {
                    tmp_array[tmp_index] = child;
                    tmp_index++;
                    child = child->next_sibling;
                }
            }

            node_array[i] = nullptr;
            i++;
        }
        Printf("\n");

        depth++;
        int j = 0;
        while(j < 100){
            node_array[j] = tmp_array[j];
            tmp_array[j] = nullptr;
            j++;
        }
    }
}

} // extern C