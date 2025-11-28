#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "BENSCHILLIBOWL.h"

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    const char* menu[] = {
        "Chili Bowl",
        "Half Smoke",
        "Cheese Fries",
        "Bacon Burger",
        "Hot Dog",
        "Veggie Chili",
        "Onion Rings",
        "Milkshake"
    };
    int num_items = sizeof(menu) / sizeof(menu[0]);
    return (MenuItem) menu[rand() % num_items];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL* bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));
    if (!bcb) {
        printf("Failed to allocate memory for restaurant.\n");
        return NULL;
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;
    bcb->is_open = true;

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    printf("Restaurant is now open! Max size: %d, Expected orders: %d\n",
           max_size, expected_num_orders);

    return bcb;
}

/* Helper function: check if restaurant queue is full */
bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

/* Helper: Add an order to the end of queue */
void AddOrderToBack(Order** orders, Order* order) {
    if (*orders == NULL) {
        *orders = order;
    } else {
        Order* temp = *orders;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = order;
    }
}

/* Add an order to the restaurant queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);

    // Wait while restaurant queue is full
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    // Assign order number
    order->order_number = bcb->next_order_number++;
    order->next = NULL;

    // Add to queue
    AddOrderToBack(&bcb->orders, order);
    bcb->current_size++;

    // Signal a cook that an order is ready
    pthread_cond_signal(&bcb->can_get_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order->order_number;
}

/* Get (remove) an order from the restaurant queue */
Order* GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    // Wait until there is an order or restaurant is closing
    while (bcb->current_size == 0 && bcb->orders_handled < bcb->expected_num_orders) {
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    // If all expected orders handled, stop
    if (bcb->orders_handled >= bcb->expected_num_orders) {
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }

    // Remove first order from queue
    Order* order = bcb->orders;
    if (order != NULL) {
        bcb->orders = order->next;
        bcb->current_size--;
        bcb->orders_handled++;
    }

    // Signal customers that space is available
    pthread_cond_signal(&bcb->can_add_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order;
}

/* Close the restaurant and free resources */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);
    bcb->is_open = false;
    pthread_cond_broadcast(&bcb->can_get_orders);
    pthread_mutex_unlock(&bcb->mutex);

    if (bcb->orders_handled != bcb->expected_num_orders) {
        printf("Warning: Expected %d orders but handled %d!\n",
               bcb->expected_num_orders, bcb->orders_handled);
    } else {
        printf("All %d orders handled successfully!\n", bcb->orders_handled);
    }

    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    free(bcb);

    printf("Restaurant is now closed.\n");
}
