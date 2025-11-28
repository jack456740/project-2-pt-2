#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "BENSCHILLIBOWL.h"

// Adjustable parameters
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS (NUM_CUSTOMERS * ORDERS_PER_CUSTOMER)

// Global restaurant pointer
BENSCHILLIBOWL *bcb;

/**
 * Thread function for a customer.
 * Each customer places several orders.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        // Allocate memory for the order
        Order* order = (Order*) malloc(sizeof(Order));
        if (!order) {
            printf("Customer %d: Failed to allocate memory for order.\n", customer_id);
            continue;
        }

        // Pick a random menu item and populate the order
        order->menu_item = PickRandomMenuItem();
        order->customer_id = customer_id;
        order->next = NULL;

        // Add order to the restaurant queue
        int order_number = AddOrder(bcb, order);

        printf("Customer %d placed order #%d: %s\n",
               customer_id, order_number, order->menu_item);

        // Small delay between orders to simulate real customers
        usleep(10000);
    }

    return NULL;
}

/**
 * Thread function for a cook.
 * Each cook retrieves and fulfills orders until no more are left.
 */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
    int orders_fulfilled = 0;

    while (1) {
        // Retrieve an order
        Order* order = GetOrder(bcb);

        // No orders left — exit loop
        if (order == NULL)
            break;

        // Simulate time to prepare the order
        printf("Cook #%d is preparing order #%d for customer %d: %s\n",
               cook_id, order->order_number, order->customer_id, order->menu_item);

        usleep(50000); // simulate 50ms prep time

        printf("Cook #%d completed order #%d\n", cook_id, order->order_number);

        // Free memory for the completed order
        free(order);
        orders_fulfilled++;
    }

    printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
    return NULL;
}

/**
 * Program entry point.
 * Initializes the restaurant, starts threads, waits for them, then closes the restaurant.
 */
int main() {
    srand(time(NULL));

    // Open the restaurant
    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);
    if (!bcb) {
        printf("Error: Could not open restaurant.\n");
        return 1;
    }

    pthread_t customer_threads[NUM_CUSTOMERS];
    pthread_t cook_threads[NUM_COOKS];

    printf("Creating %d customer threads and %d cook threads...\n", NUM_CUSTOMERS, NUM_COOKS);

    // Start cook threads first
    for (int i = 0; i < NUM_COOKS; i++) {
        if (pthread_create(&cook_threads[i], NULL, BENSCHILLIBOWLCook, (void*)(long)(i + 1)) != 0) {
            perror("Failed to create cook thread");
            return 1;
        }
    }

    // Start customer threads
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if (pthread_create(&customer_threads[i], NULL, BENSCHILLIBOWLCustomer, (void*)(long)(i + 1)) != 0) {
            perror("Failed to create customer thread");
            return 1;
        }
        usleep(5000); // simulate staggered arrivals
    }

    // Wait for all customers to finish
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customer_threads[i], NULL);
    }
    printf("All customers have finished placing orders.\n");

    // Wait for all cooks to finish
    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cook_threads[i], NULL);
    }
    printf("All cooks have finished fulfilling orders.\n");

    // Close the restaurant
    CloseRestaurant(bcb);

    printf("Simulation complete.\n");
    printf("Expected total orders: %d\n", EXPECTED_NUM_ORDERS);
    return 0;
}
