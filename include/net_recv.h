#pragma once

struct net_rx *net_rx_create(char *interface, int port);

void net_rx_destroy(struct net_rx *netrx);

void net_rx_add(struct net_rx *netrx, char *group);
void net_rx_leave(struct net_rx *netrx, char *group);
