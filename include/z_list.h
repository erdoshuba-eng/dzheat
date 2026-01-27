/*
 * list.h
 *
 *  Created on: May 16, 2021
 *      Author: huba
 */

#ifndef Z_LIST_H_
#define Z_LIST_H_

#include <Arduino.h>

template<typename E>
class ZList {
public:
  class Node;
  class Iterator;

  ZList();
  ~ZList();

  E push(E e);
  Iterator getIterator();
  void clear();
  int getCount() const { return count; } // returns the elements count in the list

  class Node {
  public:
    E element;
    Node* next;
    Node();
    Node(E element);
    Node(E element, Node* next);
  };

  class Iterator {
  public:
    Iterator(Node* node);
    bool hasNext() const;
    E getElement() { return node->element; }
    Node* getNode() const { return node; }
//    E next();
    void next() { node = node->next; }
  protected:
    Node* node;
  };

protected:
  Node* head;
  Node* tail;
  int count;
};

/**
 * Constructor
 */
template<typename E>
ZList<E>::ZList() {
  head = NULL;
  tail = NULL;
  count = 0;
}

/**
 * Add a node to the end of the list
 *
 * returns {E} - the inserted element
 */
template<typename E>
E ZList<E>::push(E e) {
  Node* node = new Node(e); // the new node's next is NULL
  if (head == NULL) {
    // when the list is empty let head and tail just point to the new node
    head = node;
    tail = node;
  }
  else {
    tail->next = node; // let the current last element next point to the new node
    tail = node; // the new node becomes the last element
  }
  count++;
  return e;
}

template<typename E>
typename ZList<E>::Iterator ZList<E>::getIterator() {
//  return Iterator(&this->head);
  return Iterator(head);
}

template<typename E>
void ZList<E>::clear() {
  while (count > 0) {
    Node* node = head; // point to the first node of the list
    head = head->next; // move the head to the next node
    delete node; // destroy the first node
    count--; // update list length
  }
  tail = NULL; // clear tail, head is already NULL
}

/**
 * Destructor, clears the list
 */
template<typename E>
ZList<E>::~ZList() {
  clear();
}

// ************** Node class

template<typename E>
ZList<E>::Node::Node() {
  next = NULL;
}

template<typename E>
ZList<E>::Node::Node(E element) : element(element) {
  next = NULL;
}

template<typename E>
ZList<E>::Node::Node(E element, Node* next) : element(element), next(next) {
}

// ************** Iterator class

template<typename E>
ZList<E>::Iterator::Iterator(Node* node) : node(node) {
}

template<typename E>
bool ZList<E>::Iterator::hasNext() const {
//  if (node == NULL) { return false; }
  return node != NULL;
}

#endif /* Z_LIST_H_ */
