#include <stdlib.h>
#include "my402list.h"

int My402ListLength(My402List *list) {
    return list->num_members;
}

int My402ListEmpty(My402List *list) {
    return My402ListLength(list) == 0;
}

int My402ListAppend(My402List *list, void *obj) {
    My402ListElem *newElem;
    if ((newElem = (My402ListElem *)malloc(sizeof(My402ListElem))) == NULL) {
        return FALSE;
    }
    
    newElem->obj = obj;
    
    if (My402ListEmpty(list)) {
        list->anchor.next = list->anchor.prev = newElem;
        newElem->next = newElem->prev = &list->anchor;
    } else {
        My402ListElem *lastElem = My402ListLast(list);
        lastElem->next = newElem;
        newElem->prev = lastElem;
        newElem->next = &list->anchor;
        list->anchor.prev = newElem;
    }
    
    (list->num_members)++;
    
    return TRUE;
}

int My402ListPrepend(My402List *list, void *obj) {
    My402ListElem *newElem;
    if ((newElem = (My402ListElem *)malloc(sizeof(My402ListElem))) == NULL) {
        return FALSE;
    }
    
    newElem->obj = obj;
    
    if (My402ListEmpty(list)) {
        list->anchor.next = list->anchor.prev = newElem;
        newElem->next = newElem->prev = &list->anchor;
        newElem->obj = obj;
    } else {
        My402ListElem *firstElem = My402ListFirst(list);
        firstElem->prev = newElem;
        newElem->next = firstElem;
        newElem->prev = &list->anchor;
        list->anchor.next = newElem;
    }
    
    (list->num_members)++;
    
    return TRUE;
}

void My402ListUnlink(My402List *list, My402ListElem *elem) {
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    
    free(elem);
    
    (list->num_members)--;
}

void My402ListUnlinkAll(My402List *list) {
    while (list->anchor.next != &list->anchor) {
        My402ListUnlink(list, list->anchor.next);
    }
}

int My402ListInsertAfter(My402List *list, void *obj, My402ListElem *elem) {
    My402ListElem *newElem;
    if ((newElem = (My402ListElem *)malloc(sizeof(My402ListElem))) == NULL) {
        return FALSE;
    }
    
    newElem->next = elem->next;
    newElem->prev = elem;
    newElem->obj = obj;
    
    elem->next->prev = newElem;
    elem->next = newElem;
    
    (list->num_members)++;
    
    return TRUE;
}

int My402ListInsertBefore(My402List *list, void *obj, My402ListElem *elem) {
    My402ListElem *newElem;
    if ((newElem = (My402ListElem *)malloc(sizeof(My402ListElem))) == NULL) {
        return FALSE;
    }
    
    newElem->next = elem;
    newElem->prev = elem->prev;
    newElem->obj = obj;
    
    elem->prev->next = newElem;
    elem->prev = newElem;
    
    (list->num_members)++;
    
    return TRUE;
}

My402ListElem *My402ListFirst(My402List *list) {
    return My402ListEmpty(list) ? NULL : list->anchor.next;
}

My402ListElem *My402ListLast(My402List *list) {
    return My402ListEmpty(list) ? NULL : list->anchor.prev;
}

My402ListElem *My402ListNext(My402List *list, My402ListElem *elem) {
    return My402ListLast(list) == elem ? NULL : elem->next;
}

My402ListElem *My402ListPrev(My402List *list, My402ListElem *elem) {
    return My402ListFirst(list) == elem ? NULL : elem->prev;
}

My402ListElem *My402ListFind(My402List *list, void *obj) {
    My402ListElem *curElem = My402ListFirst(list);
    while (curElem != &list->anchor) {
        if (curElem->obj == obj) {
            return curElem;
        }
        curElem = My402ListNext(list, curElem);
    }
    return NULL;
}

int My402ListInit(My402List *list) {
    list->num_members = 0;
    list->anchor.next = &list->anchor;
    list->anchor.prev = &list->anchor;
    
    return TRUE;
}











