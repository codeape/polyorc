/*
 Originally created by Oscar Norlander (codeape)
 ...................................................................
 @@@@@@@    @@@@@@   @@@       @@@ @@@   @@@@@@   @@@@@@@    @@@@@@@
 @@@@@@@@  @@@@@@@@  @@@       @@@ @@@  @@@@@@@@  @@@@@@@@  @@@@@@@@
 @@!  @@@  @@!  @@@  @@!       @@! !@@  @@!  @@@  @@!  @@@  !@@
 !@!  @!@  !@!  @!@  !@!       !@! @!!  !@!  @!@  !@!  @!@  !@!
 @!@@!@!   @!@  !@!  @!!        !@!@!   @!@  !@!  @!@!!@!   !@!
 !!@!!!    !@!  !!!  !!!         @!!!   !@!  !!!  !!@!@!    !!!
 !!:       !!:  !!!  !!:         !!:    !!:  !!!  !!: :!!   :!!
 :!:       :!:  !:!   :!:        :!:    :!:  !:!  :!:  !:!  :!:
  ::       ::::: ::   :: ::::     ::    ::::: ::  ::   :::   ::: :::
  :         : :  :   : :: : :     :      : :  :    :   : :   :: :: :
 ...................................................................
 Polyorc is under BSD 2-Clause License (see LICENSE file)
*/

#include "polyorcbintree.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * An out of the box string comparator
 *
 * @author Oscar Norlander
 *
 * @param current The key of the node the new node is compared.
 *                with.
 * @param added The key of the new node.
 *
 * @return int Returns 0 if equal, a negativ value if added is
 *         smaller then current and a positive value if added is
 *         bigger then current.
 */
int bintree_streq(void *current, void *added) {
    // We check if the added is bigger than current (see strcmp)
    return strcmp((const char*)added, (const char*)current);
}

/**
 * An out of the box int comparator
 *
 * @author Oscar Norlander
 *
 * @param current The key of the node the new node is compared.
 *                with.
 * @param added he key of the new node.
 *
 * @return int Returns 0 if equal, a negativ value if added is
 *         smaller then current and a positive value if added is
 *         bigger then current.
 */
int bintree_inteq(void *current, void *added) {
    // We check if the added is bigger than current (see strcmp)
    int int_added = *((int *)added);
    int int_current = *((int *)current);
    return int_added - int_current;
}

/**
 * Initialize a bintree. Must be done before any other bintree_*
 * can be called.
 *
 * @author Oscar Norlander
 *
 * @param root
 * @param equals A comparator function
 * @param free_value A free function (used by bintree_delete and
 *                   bintree_free).
 */
void bintree_init(bintree_root *root, bintree_equal_fptr equals,
                  bintree_free_fptr free_value)
{
    memset(root, 0, sizeof(*root));
    root->_equals = equals;
    root->_free_value = free_value;
}

int _bintree_add(bintree_equal_fptr eq, bintree_node **node, void *key,
                 void *value)
{
    // We have found a place for the new node
    if (0 == (*node)) {
        bintree_node *tmp = malloc(sizeof(**node));
        memset(tmp, 0, sizeof(**node));
        tmp->key = key;
        tmp->value = value;
        (*node) = tmp;
        return 1;
    }

    // Find out the nodes next path or if a node with the same key exist
    int compare = eq((*node)->key, key);
    if (0 < compare) {
        // bigger
        return _bintree_add(eq, &((*node)->more_n), key, value);
    } else if (0 > compare) {
        // smaller
        return _bintree_add(eq, &((*node)->less_n), key, value);
    } else {
        return 0;
    }
}

/**
 * Add a key with a value to the bintree.
 *
 * @author Oscar Norlander
 *
 * @param root The root of the tree.
 * @param key The key for the value.
 * @param value The value to add to the tree.
 *
 * @return int returns 0 if key already exists and a nin zero
 *         value if not. If 0 is returned the value was not
 *         added to the tree.
 */
int bintree_add(bintree_root *root, void *key, void *value) {
    int ret = _bintree_add(root->_equals, &(root->_root_node), key, value);
    if (0 != ret) {
        root->node_count++;
    }
    return ret;
}

void * _bintree_find(bintree_equal_fptr eq, bintree_node *node, void *key) {
    // The key was not found
    if (0 == node) {
        return 0;
    }

    int compare = eq(node->key, key);
    if (0 < compare) {
        // bigger
        return _bintree_find(eq, node->more_n, key);
    } else if (0 > compare) {
        // smaller
        return _bintree_find(eq, node->less_n, key);
    } else {
        // Found the key
        return node->value;
    }
}

/**
 * Find a key in the bintree and return it associated value.
 *
 * @author Oscar Norlander
 *
 * @param root The root of the tree.
 * @param key The key for the value.
 *
 * @return void* returns a reference to the value if found
 *         otherwise it returns 0.
 */
void * bintree_find(bintree_root *root, void *key) {
    return _bintree_find(root->_equals, root->_root_node, key);
}


void _bintree_reenter(bintree_equal_fptr eq, bintree_node **root,
                      bintree_node *node)
{
    // Do we have a node to add?
    if (0 == node) {
        return;
    }
    // We have a place to insert the node so lets do it
    if (0 == (*root)) {
        (*root) = node;
        return;
    }

    int compare = eq((*root)->key, node->key);
    if (0 < compare) {
        // bigger
        _bintree_reenter(eq, &((*root)->more_n), node);
    } else if (0 > compare) {
        // smaller
        _bintree_reenter(eq, &((*root)->less_n), node);
    } else {
        // Uggly, yes, but we do not allow insert for equal keys so this
        // should never happen.
        assert(0 != compare);
    }
}

void * _bintree_delete(bintree_equal_fptr eq, bintree_free_fptr dl,
                      bintree_node **node, void *key, int *ncount)
{
    // The key was not found
    if (0 == (*node)) {
        return 0;
    }

    int compare = eq((*node)->key, key);
    if (0 < compare) {
        // bigger
        return _bintree_delete(eq, dl, &((*node)->more_n), key, ncount);
    } else if (0 > compare) {
        // smaller
        return _bintree_delete(eq, dl, &((*node)->less_n), key, ncount);
    } else {
        // The key was found lets delete
        bintree_node *tmp = (*node);
        void *ret = tmp->value;
        dl(&(tmp->key),&(tmp->value), POLY_DELETE);
        (*node) = tmp->less_n;
        // If we have more nodes, lets put them back.
        _bintree_reenter(eq, node, tmp->more_n);
        free(tmp);
        // Decrese the node count
        (*ncount) = (*ncount) - 1;
        return ret;
    }
}
/**
 * Delete a key from the bin tree. POLY_DELETE will be sent to
 * the free_value function.
 *
 * @author Oscar Norlander
 *
 * @param root The root of the tree.
 * @param key The key for the value.
 *
 * @return void* returns a reference to the value if found
 *         otherwise it returns 0.
 */
void * bintree_delete(bintree_root *root, void *key) {
    return _bintree_delete(root->_equals, root->_free_value,
                         &(root->_root_node), key, &(root->node_count));
}

void _bintree_free(bintree_node **node, bintree_free_fptr dl) {
    if (0 != (*node)) {
        _bintree_free(&((*node)->less_n), dl);
        _bintree_free(&((*node)->more_n), dl);
        dl(&((*node)->key), &((*node)->value), POLY_FREE_ALL);
        free(*node);
        (*node) = 0;
    }
}

/**
 * Frees everything in the tree. POLY_FREE_ALL will be sent to
 * the free_value function.
 *
 * @author Oscar Norlander
 *
 * @param root The root of the tree.
 */
void bintree_free(bintree_root *root) {
    _bintree_free(&(root->_root_node), root->_free_value);
    memset(root, 0, sizeof(*root));
}
