#include "queue.h"

int queueLoad (Queue Q, int (*eval)(void*)) {

  int total = 0;

  pthread_rwlock_rdlock(&(Q->lock));
  Bin current = Q->front;
  for (int i = 0; i < Q->length; i++) {
    total += (*eval)(current->element);
    current = current->next;
  }
  pthread_rwlock_unlock(&(Q->lock));

  return total;

}

int queueEnqueue (Queue Q, void* e) {

  pthread_rwlock_wrlock(&(Q->lock));
  if (Q->length == Q->maxSize) {
    pthread_rwlock_unlock(&(Q->lock));
    return -1;
  }

  Bin newBin = malloc(sizeof(struct Bin));
  newBin->element = e;
  newBin->next = NULL;

  if (Q->length == 0) {
    Q->front = newBin;
    Q->back = newBin;
  }
  else {
    Bin currentBack = Q->back;
    currentBack->next = newBin;
    Q->back = newBin;
  }
  Q->length++;
  pthread_rwlock_unlock(&(Q->lock));

  return 0;

}

void* queueDequeue (Queue Q) {

  pthread_rwlock_wrlock(&(Q->lock));
  if (Q->length == 0) {
    pthread_rwlock_unlock(&(Q->lock));
    return NULL;
  }

  Bin removedBin = Q->front;
  Q->front = removedBin->next;
  if (Q->length == 1) {
    Q->back = NULL;
  }
  Q->length--;
  pthread_rwlock_unlock(&(Q->lock));

  return removedBin->element;

}

int queueLength (Queue Q) {

  pthread_rwlock_rdlock(&(Q->lock));
  int l = Q->length;
  pthread_rwlock_unlock(&(Q->lock));
  return l;

}

Queue queueInit (int maxSize) {

  Queue Q = malloc(sizeof(struct Queue));
  Q->front = NULL;
  Q->back = NULL;
  Q->length = 0;
  Q->maxSize = maxSize;
  pthread_rwlock_init(&(Q->lock), NULL);
  return Q;

}

void queueFree (Queue Q) {

  free(Q);

}


