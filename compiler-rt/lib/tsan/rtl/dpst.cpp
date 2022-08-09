#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "tsan_rtl.h"
#include "concurrency_vector.h"

namespace __tsan{

extern ConcurrencyVector step_nodes;

// dpst data structure
static DPST dpst;

char node_char[6] = {'R','F','A','f','S','W'};

// static int node_index_base = 0;
// atomic_uint32_t *atomic_node_index = reinterpret_cast<atomic_uint32_t *>(&node_index_base);

// static u32 step_index_base = 1;
// atomic_uint32_t *atomic_step_index = reinterpret_cast<atomic_uint32_t *>(&step_index_base);

extern "C" {
INTERFACE_ATTRIBUTE
void putNodeInCurThread(TreeNode* node){
    // Printf("new task node index is %d \n", node->index);
    ThreadState* current_thread = cur_thread();
    current_thread->current_task_node = node;
}

/**
 * @brief  Create a new tree node
 * @note   
 * @retval The newly created tree node
 */
TreeNode *newtreeNode()
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

    node->corresponding_step_index = -2;

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
TreeNode *insert_tree_node(NodeType nodeType, TreeNode *parent){
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

    if(node->depth > dpst.height){
        dpst.height = node->depth;
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
TreeNode *insert_leaf(TreeNode *task_node){
    int preceeding_taskwait = (task_node->children_list_head) ? task_node->children_list_tail->preceeding_taskwait : -1;
    TreeNode &n = step_nodes.EmplaceBack(task_node->corresponding_task_id, STEP, task_node->depth + 1, task_node->number_of_child, preceeding_taskwait, task_node);   
    TreeNode *new_step = &n;
    task_node->number_of_child += 1;
    
    if(task_node->children_list_head == nullptr){
        task_node->children_list_head = new_step;
        task_node->children_list_tail = new_step;
    }
    else{
        task_node->children_list_tail->next_sibling = new_step;
        task_node->children_list_tail = new_step;
    }

    if(new_step->depth > dpst.height){
        dpst.height = new_step->depth;
    }

    // u32 step_index = atomic_load(atomic_step_index, memory_order_acquire);
    // atomic_store(atomic_step_index,step_index+1,memory_order_consume);

    // new_step->corresponding_step_index = step_index;
    // step_nodes[step_index] = new_step;
    // Printf("insert into %d\n", step_index);

    return new_step;
}


int get_dpst_height(){
    return dpst.height;
}

INTERFACE_ATTRIBUTE
void printDPST(){
    TreeNode *node_array[100]{};
    TreeNode *tmp_array[100]{};
    node_array[0] = dpst.root;
    int depth = 0;

    while (node_array[0] != nullptr)
    {
        Printf("depth %d:   ",depth);
        int tmp_index = 0;
        int i = 0;
        while (i < 100)
        {
            TreeNode *node = node_array[i];
            if(node == nullptr){
                //Printf("   ");
            }
            else{
                Printf("%c (i:%p) ",node_char[node->this_node_type],node);
                if(node->parent != nullptr){
                    Printf("(p:%p)    ",node->parent);
                }
                TreeNode *child = node->children_list_head;
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


INTERFACE_ATTRIBUTE
void DPSTinfo(){
    Printf("\n");
    printDPST();

    Printf("\n");
    // Printf("DPST height is %d, number of nodes is %d, number of step nodes is %d \n", dpst.height, 
    //     atomic_load(atomic_node_index, memory_order_acquire), atomic_load(atomic_step_index, memory_order_acquire));
}


INTERFACE_ATTRIBUTE
void set_ompt_ready(bool b){
    ompt_ready = b;
}

} // extern C
} // namespace __tsan