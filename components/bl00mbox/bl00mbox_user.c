// SPDX-License-Identifier: CC0-1.0
#include "bl00mbox_user.h"

static uint64_t bl00mbox_bud_index = 1;
bl00mbox_bud_t *bl00mbox_channel_get_bud_by_index(uint8_t channel,
                                                  uint32_t index);

uint16_t bl00mbox_channel_buds_num(uint8_t channel) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    uint16_t ret = 0;
    if (chan->buds != NULL) {
        bl00mbox_bud_t *last = chan->buds;
        ret++;
        while (last->chan_next != NULL) {
            last = last->chan_next;
            ret++;
        }
    }
    return ret;
}

uint64_t bl00mbox_channel_get_bud_by_list_pos(uint8_t channel, uint32_t pos) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    uint16_t ret = 0;
    if (chan->buds != NULL) {
        bl00mbox_bud_t *last = chan->buds;
        if (pos == ret) return last->index;
        ret++;
        while (last->chan_next != NULL) {
            last = last->chan_next;
            if (pos == ret) return last->index;
            ret++;
        }
    }
    return 0;
}

uint16_t bl00mbox_channel_conns_num(uint8_t channel) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    uint16_t ret = 0;
    if (chan->connections != NULL) {
        bl00mbox_connection_t *last = chan->connections;
        ret++;
        while (last->chan_next != NULL) {
            last = last->chan_next;
            ret++;
        }
    }
    return ret;
}

uint16_t bl00mbox_channel_mixer_num(uint8_t channel) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    uint16_t ret = 0;
    if (chan->root_list != NULL) {
        bl00mbox_channel_root_t *last = chan->root_list;
        ret++;
        while (last->next != NULL) {
            last = last->next;
            ret++;
        }
    }
    return ret;
}

uint64_t bl00mbox_channel_get_bud_by_mixer_list_pos(uint8_t channel,
                                                    uint32_t pos) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    uint16_t ret = 0;
    if (chan->buds != NULL) {
        bl00mbox_channel_root_t *last = chan->root_list;
        if (pos == ret) return last->con->source_bud->index;
        ret++;
        while (last->next != NULL) {
            last = last->next;
            if (pos == ret) return last->con->source_bud->index;
            ret++;
        }
    }
    return 0;
}

uint32_t bl00mbox_channel_get_signal_by_mixer_list_pos(uint8_t channel,
                                                       uint32_t pos) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    uint16_t ret = 0;
    if (chan->buds != NULL) {
        bl00mbox_channel_root_t *last = chan->root_list;
        if (pos == ret) return last->con->signal_index;
        ret++;
        while (last->next != NULL) {
            last = last->next;
            if (pos == ret) return last->con->signal_index;
            ret++;
        }
    }
    return 0;
}

uint16_t bl00mbox_channel_subscriber_num(uint8_t channel, uint64_t bud_index,
                                         uint16_t signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return 0;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return 0;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, signal_index);
    if (sig == NULL) return 0;
    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)sig->buffer;  // buffer sits on top of struct
    if (conn == NULL) return 0;

    uint16_t ret = 0;
    if (conn->subs != NULL) {
        bl00mbox_connection_subscriber_t *last = conn->subs;
        ret++;
        while (last->next != NULL) {
            last = last->next;
            ret++;
        }
    }
    return ret;
}

uint64_t bl00mbox_channel_get_bud_by_subscriber_list_pos(uint8_t channel,
                                                         uint64_t bud_index,
                                                         uint16_t signal_index,
                                                         uint8_t pos) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return 0;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return 0;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, signal_index);
    if (sig == NULL) return 0;
    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)sig->buffer;  // buffer sits on top of struct
    if (conn == NULL) return 0;

    uint16_t ret = 0;
    if (conn->subs != NULL) {
        bl00mbox_connection_subscriber_t *last = conn->subs;
        if (pos == ret) return (last->type == 0) ? last->bud_index : 0;
        ret++;
        while (last->next != NULL) {
            last = last->next;
            if (pos == ret) return (last->type == 0) ? last->bud_index : 0;
            ret++;
        }
    }
    return 0;
}

int32_t bl00mbox_channel_get_signal_by_subscriber_list_pos(
    uint8_t channel, uint64_t bud_index, uint16_t signal_index, uint8_t pos) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return 0;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return 0;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, signal_index);
    if (sig == NULL) return 0;
    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)sig->buffer;  // buffer sits on top of struct
    if (conn == NULL) return 0;

    uint16_t ret = 0;
    if (conn->subs != NULL) {
        bl00mbox_connection_subscriber_t *last = conn->subs;
        if (pos == ret) return (last->type == 0) ? last->signal_index : -1;
        ret++;
        while (last->next != NULL) {
            last = last->next;
            if (pos == ret) return (last->type == 0) ? last->signal_index : -1;
            ret++;
        }
    }
    return 0;
}

uint64_t bl00mbox_channel_get_source_bud(uint8_t channel, uint64_t bud_index,
                                         uint16_t signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return 0;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return 0;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, signal_index);
    if (sig == NULL) return 0;
    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)sig->buffer;  // buffer sits on top of struct
    if (conn == NULL) return 0;
    return conn->source_bud->index;
}

uint16_t bl00mbox_channel_get_source_signal(uint8_t channel, uint64_t bud_index,
                                            uint16_t signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return 0;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return 0;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, signal_index);
    if (sig == NULL) return 0;
    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)sig->buffer;  // buffer sits on top of struct
    if (conn == NULL) return 0;
    return conn->signal_index;
}

static bl00mbox_connection_t *create_connection(uint8_t channel) {
    bl00mbox_connection_t *ret = malloc(sizeof(bl00mbox_connection_t));
    if (ret == NULL) return NULL;
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    ret->chan_next = NULL;
    ret->subs = NULL;
    ret->channel = channel;

    if (chan->connections != NULL) {
        bl00mbox_connection_t *last = chan->connections;
        while (last->chan_next != NULL) {
            last = last->chan_next;
        }
        last->chan_next = ret;
    } else {
        chan->connections = ret;
    }
    return ret;
}

static bool weak_delete_connection(bl00mbox_connection_t *conn) {
    if (conn->subs != NULL) return false;

    // nullify source bud connection;
    bl00mbox_bud_t *bud = conn->source_bud;
    if (bud != NULL) {
        radspa_signal_t *tx =
            radspa_signal_get_by_index(bud->plugin, conn->signal_index);
        if (tx != NULL) {
            bl00mbox_audio_waitfor_pointer_change(&(tx->buffer), NULL);
        }
    }

    // pop from channel list
    bl00mbox_channel_t *chan = bl00mbox_get_channel(conn->channel);
    if (chan->connections != NULL) {
        if (chan->connections != conn) {
            bl00mbox_connection_t *prev = chan->connections;
            while (prev->chan_next != conn) {
                prev = prev->chan_next;
                if (prev->chan_next == NULL) {
                    break;
                }
            }
            if (prev->chan_next != NULL)
                bl00mbox_audio_waitfor_pointer_change(&(prev->chan_next),
                                                      conn->chan_next);
        } else {
            bl00mbox_audio_waitfor_pointer_change(&(chan->connections),
                                                  conn->chan_next);
        }
    }

    free(conn);
    return true;
}

bl00mbox_bud_t *bl00mbox_channel_get_bud_by_index(uint8_t channel,
                                                  uint32_t index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return NULL;
    if (chan->buds == NULL) return NULL;
    bl00mbox_bud_t *bud = chan->buds;
    while (true) {
        if (bud->index == index) break;
        bud = bud->chan_next;
        if (bud == NULL) break;
    }
    return bud;
}

bl00mbox_bud_t *bl00mbox_channel_new_bud(uint8_t channel, uint32_t id,
                                         uint32_t init_var) {
    /// creates a new bud instance of the plugin with descriptor id "id" and the
    /// initialization variable "init_var" and appends it to the plugin list of
    /// the corresponding channel. returns pointer to the bud if successfull,
    /// else NULL.
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return NULL;
    radspa_descriptor_t *desc =
        bl00mbox_plugin_registry_get_descriptor_from_id(id);
    if (desc == NULL) return NULL;
    bl00mbox_bud_t *bud = malloc(sizeof(bl00mbox_bud_t));
    if (bud == NULL) return NULL;
    radspa_t *plugin = desc->create_plugin_instance(init_var);
    if (plugin == NULL) {
        free(bud);
        return NULL;
    }

    bud->plugin = plugin;
    bud->channel = channel;
    // TODO: look for empty indices
    bud->index = bl00mbox_bud_index;
    bl00mbox_bud_index++;
    bud->chan_next = NULL;

    // append to channel bud list
    if (chan->buds == NULL) {
        chan->buds = bud;
    } else {
        bl00mbox_bud_t *last = chan->buds;
        while (last->chan_next != NULL) {
            last = last->chan_next;
        }
        last->chan_next = bud;
    }
    bl00mbox_channel_event(channel);
    return bud;
}

bool bl00mbox_channel_delete_bud(uint8_t channel, uint32_t bud_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;

    // disconnect all signals
    uint16_t num_signals =
        bl00mbox_channel_bud_get_num_signals(channel, bud_index);
    for (uint16_t i = 0; i < num_signals; i++) {
        bl00mbox_channel_disconnect_signal(channel, bud_index, i);
    }

    // pop from channel bud list
    bl00mbox_bud_t *seek = chan->buds;
    bool free_later = false;
    if (chan->buds != NULL) {
        bl00mbox_bud_t *prev = NULL;
        while (seek != NULL) {
            if (seek->index == bud_index) {
                break;
            }
            prev = seek;
            seek = seek->chan_next;
        }
        if (seek != NULL) {
            if (prev != NULL) {
                bl00mbox_audio_waitfor_pointer_change(&(prev->chan_next),
                                                      seek->chan_next);
            } else {
                bl00mbox_audio_waitfor_pointer_change(&(chan->buds),
                                                      seek->chan_next);
            }
            free_later = true;
        }
    }

    bud->plugin->descriptor->destroy_plugin_instance(bud->plugin);
    if (free_later) free(seek);
    return true;
}

bool bl00mbox_channel_clear(uint8_t channel) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = chan->buds;
    while (bud != NULL) {
        bl00mbox_bud_t *bud_next = bud->chan_next;
        bl00mbox_channel_delete_bud(channel, bud->index);
        bud = bud_next;
    }
    return true;
}

bool bl00mbox_channel_connect_signal_to_output_mixer(
    uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *tx =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (tx == NULL) return false;
    if (!(tx->hints & RADSPA_SIGNAL_HINT_OUTPUT)) return false;

    bl00mbox_channel_root_t *root = malloc(sizeof(bl00mbox_channel_root_t));
    if (root == NULL) return false;
    bl00mbox_connection_subscriber_t *sub =
        malloc(sizeof(bl00mbox_connection_subscriber_t));
    if (sub == NULL) {
        free(root);
        return false;
    }

    bl00mbox_connection_t *conn;
    if (tx->buffer == NULL) {  // doesn't feed a buffer yet
        conn = create_connection(channel);
        if (conn == NULL) {
            free(sub);
            free(root);
            return false;
        }
        // set up new connection
        conn->signal_index = bud_signal_index;
        conn->source_bud = bud;
        tx->buffer = conn->buffer;
    } else {
        conn = (bl00mbox_connection_t *)
                   tx->buffer;  // buffer sits on top of struct
        if (conn->subs != NULL) {
            bl00mbox_connection_subscriber_t *seek = conn->subs;
            while (seek != NULL) {
                if (seek->type == 1) {
                    free(root);
                    free(sub);
                    return false;  // already connected
                }
                seek = seek->next;
            }
        }
    }

    sub->type = 1;
    sub->bud_index = bud_index;
    sub->signal_index = bud_signal_index;
    sub->next = NULL;
    if (conn->subs == NULL) {
        conn->subs = sub;
    } else {
        bl00mbox_connection_subscriber_t *seek = conn->subs;
        while (seek->next != NULL) {
            seek = seek->next;
        }
        seek->next = sub;
    }

    root->con = conn;
    root->next = NULL;

    if (chan->root_list == NULL) {
        chan->root_list = root;
    } else {
        bl00mbox_channel_root_t *last_root = chan->root_list;
        while (last_root->next != NULL) {
            last_root = last_root->next;
        }
        last_root->next = root;
    }

    bl00mbox_channel_event(channel);
    return true;
}

bool bl00mbox_channel_disconnect_signal_from_output_mixer(
    uint8_t channel, uint32_t bud_index, uint32_t bud_signal_index) {
    // TODO
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *tx =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (tx == NULL) return false;
    if (tx->buffer == NULL) return false;
    if (!(tx->hints & RADSPA_SIGNAL_HINT_OUTPUT)) return false;

    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)tx->buffer;  // buffer sits on top of struct
    if (conn == NULL) return false;           // not connected

    bl00mbox_channel_root_t *rt = chan->root_list;
    bl00mbox_channel_root_t *rt_prev = NULL;

    while (rt != NULL) {
        if (rt->con == conn) break;
        rt_prev = rt;
        rt = rt->next;
    }
    if (rt != NULL) {
        if (rt_prev == NULL) {
            bl00mbox_audio_waitfor_pointer_change(&(chan->root_list), rt->next);
        } else {
            bl00mbox_audio_waitfor_pointer_change(&(rt_prev->next), rt->next);
        }
        free(rt);
    }

    if (conn->subs != NULL) {
        bl00mbox_connection_subscriber_t *seek = conn->subs;
        bl00mbox_connection_subscriber_t *prev = NULL;
        while (seek != NULL) {
            if (seek->type == 1) {
                break;
            }
            prev = seek;
            seek = seek->next;
        }
        if (seek != NULL) {
            if (prev != NULL) {
                bl00mbox_audio_waitfor_pointer_change(&(prev->next),
                                                      seek->next);
            } else {
                bl00mbox_audio_waitfor_pointer_change(&(conn->subs),
                                                      seek->next);
            }
            free(seek);
        }
    }

    weak_delete_connection(conn);
    bl00mbox_channel_event(channel);
    return true;
}

bool bl00mbox_channel_disconnect_signal_rx(uint8_t channel,
                                           uint32_t bud_rx_index,
                                           uint32_t bud_rx_signal_index) {
    bl00mbox_bud_t *bud_rx =
        bl00mbox_channel_get_bud_by_index(channel, bud_rx_index);
    if (bud_rx == NULL) return false;  // bud index doesn't exist

    radspa_signal_t *rx =
        radspa_signal_get_by_index(bud_rx->plugin, bud_rx_signal_index);
    if (rx == NULL) return false;  // signal index doesn't exist
    if (rx->buffer == NULL) return false;
    if (!(rx->hints & RADSPA_SIGNAL_HINT_INPUT)) return false;

    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)rx->buffer;  // buffer sits on top of struct
    if (conn == NULL) return false;           // not connected

    bl00mbox_bud_t *bud_tx = conn->source_bud;
    if (bud_tx == NULL) return false;  // bud index doesn't exist
    radspa_signal_t *tx =
        radspa_signal_get_by_index(bud_tx->plugin, conn->signal_index);
    if (tx == NULL) return false;  // signal index doesn't exist

    bl00mbox_audio_waitfor_pointer_change(&(rx->buffer), NULL);

    if (conn->subs != NULL) {
        bl00mbox_connection_subscriber_t *seek = conn->subs;
        bl00mbox_connection_subscriber_t *prev = NULL;
        while (seek != NULL) {
            if ((seek->signal_index == bud_rx_signal_index) &&
                (seek->bud_index == bud_rx_index) && (seek->type == 0)) {
                break;
            }
            prev = seek;
            seek = seek->next;
        }
        if (seek != NULL) {
            if (prev != NULL) {
                bl00mbox_audio_waitfor_pointer_change(&(prev->next),
                                                      seek->next);
            } else {
                bl00mbox_audio_waitfor_pointer_change(&(conn->subs),
                                                      seek->next);
            }
            free(seek);
        }
    }

    weak_delete_connection(conn);
    bl00mbox_channel_event(channel);
    return true;
}

bool bl00mbox_channel_disconnect_signal_tx(uint8_t channel,
                                           uint32_t bud_tx_index,
                                           uint32_t bud_tx_signal_index) {
    bl00mbox_bud_t *bud_tx =
        bl00mbox_channel_get_bud_by_index(channel, bud_tx_index);
    if (bud_tx == NULL) return false;  // bud index doesn't exist

    radspa_signal_t *tx =
        radspa_signal_get_by_index(bud_tx->plugin, bud_tx_signal_index);
    if (tx == NULL) return false;  // signal index doesn't exist
    if (tx->buffer == NULL) return false;
    if (!(tx->hints & RADSPA_SIGNAL_HINT_OUTPUT)) return false;

    bl00mbox_connection_t *conn =
        (bl00mbox_connection_t *)tx->buffer;  // buffer sits on top of struct
    if (conn == NULL) return false;           // not connected

    while (conn->subs != NULL) {
        switch (conn->subs->type) {
            case 0:
                bl00mbox_channel_disconnect_signal_rx(
                    channel, conn->subs->bud_index, conn->subs->signal_index);
                break;
            case 1:
                bl00mbox_channel_disconnect_signal_from_output_mixer(
                    channel, conn->subs->bud_index, conn->subs->signal_index);
                break;
        }
    }
    bl00mbox_channel_event(channel);
    return true;
}

bool bl00mbox_channel_disconnect_signal(uint8_t channel, uint32_t bud_index,
                                        uint32_t signal_index) {
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;  // bud index doesn't exist
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, signal_index);
    if (sig == NULL) return false;  // signal index doesn't exist
    if (sig->buffer == NULL) return false;

    bl00mbox_channel_disconnect_signal_rx(channel, bud_index, signal_index);
    bl00mbox_channel_disconnect_signal_tx(channel, bud_index, signal_index);
    bl00mbox_channel_disconnect_signal_from_output_mixer(channel, bud_index,
                                                         signal_index);
    if (sig->buffer == NULL) return true;
    return false;
}

bool bl00mbox_channel_connect_signal(uint8_t channel, uint32_t bud_rx_index,
                                     uint32_t bud_rx_signal_index,
                                     uint32_t bud_tx_index,
                                     uint32_t bud_tx_signal_index) {
    bl00mbox_bud_t *bud_rx =
        bl00mbox_channel_get_bud_by_index(channel, bud_rx_index);
    bl00mbox_bud_t *bud_tx =
        bl00mbox_channel_get_bud_by_index(channel, bud_tx_index);
    if (bud_tx == NULL || bud_rx == NULL)
        return false;  // bud index doesn't exist

    radspa_signal_t *rx =
        radspa_signal_get_by_index(bud_rx->plugin, bud_rx_signal_index);
    radspa_signal_t *tx =
        radspa_signal_get_by_index(bud_tx->plugin, bud_tx_signal_index);
    if (tx == NULL || rx == NULL) return false;  // signal index doesn't exist
    if (!(rx->hints & RADSPA_SIGNAL_HINT_INPUT)) return false;
    if (!(tx->hints & RADSPA_SIGNAL_HINT_OUTPUT)) return false;

    bl00mbox_connection_t *conn;
    bl00mbox_connection_subscriber_t *sub;
    if (tx->buffer == NULL) {  // doesn't feed a buffer yet
        conn = create_connection(channel);
        if (conn == NULL) return false;  // no ram for connection
        // set up new connection
        conn->signal_index = bud_tx_signal_index;
        conn->source_bud = bud_tx;
        tx->buffer = conn->buffer;
    } else {
        if (rx->buffer == tx->buffer) return false;  // already connected
        conn = (bl00mbox_connection_t *)
                   tx->buffer;  // buffer sits on top of struct
    }

    bl00mbox_channel_disconnect_signal_rx(channel, bud_rx_index,
                                          bud_rx_signal_index);

    sub = malloc(sizeof(bl00mbox_connection_subscriber_t));
    if (sub == NULL) {
        weak_delete_connection(conn);
        return false;
    }
    sub->type = 0;
    sub->bud_index = bud_rx_index;
    sub->signal_index = bud_rx_signal_index;
    sub->next = NULL;
    if (conn->subs == NULL) {
        conn->subs = sub;
    } else {
        bl00mbox_connection_subscriber_t *seek = conn->subs;
        while (seek->next != NULL) {
            seek = seek->next;
        }
        seek->next = sub;
    }

    rx->buffer = tx->buffer;
    bl00mbox_channel_event(channel);
    return true;
}

bool bl00mbox_channel_bud_exists(uint8_t channel, uint32_t bud_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) {
        return false;
    } else {
        return true;
    }
}

char *bl00mbox_channel_bud_get_name(uint8_t channel, uint32_t bud_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    return bud->plugin->descriptor->name;
}

char *bl00mbox_channel_bud_get_description(uint8_t channel,
                                           uint32_t bud_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    return bud->plugin->descriptor->description;
}

uint32_t bl00mbox_channel_bud_get_plugin_id(uint8_t channel,
                                            uint32_t bud_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    return bud->plugin->descriptor->id;
}

uint16_t bl00mbox_channel_bud_get_num_signals(uint8_t channel,
                                              uint32_t bud_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    return bud->plugin->len_signals;
}

char *bl00mbox_channel_bud_get_signal_name(uint8_t channel, uint32_t bud_index,
                                           uint32_t bud_signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (sig == NULL) return false;
    return sig->name;
}

char *bl00mbox_channel_bud_get_signal_description(uint8_t channel,
                                                  uint32_t bud_index,
                                                  uint32_t bud_signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (sig == NULL) return false;
    return sig->description;
}

char *bl00mbox_channel_bud_get_signal_unit(uint8_t channel, uint32_t bud_index,
                                           uint32_t bud_signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (sig == NULL) return false;
    return sig->unit;
}

bool bl00mbox_channel_bud_set_signal_value(uint8_t channel, uint32_t bud_index,
                                           uint32_t bud_signal_index,
                                           int16_t value) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (sig == NULL) return false;

    sig->value = value;
    bl00mbox_channel_event(channel);
    return true;
}

int16_t bl00mbox_channel_bud_get_signal_value(uint8_t channel,
                                              uint32_t bud_index,
                                              uint32_t bud_signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (sig == NULL) return false;

    return sig->value;
}

uint32_t bl00mbox_channel_bud_get_signal_hints(uint8_t channel,
                                               uint32_t bud_index,
                                               uint32_t bud_signal_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    radspa_signal_t *sig =
        radspa_signal_get_by_index(bud->plugin, bud_signal_index);
    if (sig == NULL) return false;

    return sig->hints;
}

bool bl00mbox_channel_bud_set_table_value(uint8_t channel, uint32_t bud_index,
                                          uint32_t table_index, int16_t value) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    if (bud->plugin->plugin_table == NULL) return false;
    if (table_index >= bud->plugin->plugin_table_len) return false;
    bud->plugin->plugin_table[table_index] = value;
    bl00mbox_channel_event(channel);
    return true;
}

int16_t bl00mbox_channel_bud_get_table_value(uint8_t channel,
                                             uint32_t bud_index,
                                             uint32_t table_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    if (bud->plugin->plugin_table == NULL) return false;
    if (table_index >= bud->plugin->plugin_table_len) return false;
    return bud->plugin->plugin_table[table_index];
}

int16_t bl00mbox_channel_bud_get_table_len(uint8_t channel,
                                           uint32_t bud_index) {
    bl00mbox_channel_t *chan = bl00mbox_get_channel(channel);
    if (chan == NULL) return false;
    bl00mbox_bud_t *bud = bl00mbox_channel_get_bud_by_index(channel, bud_index);
    if (bud == NULL) return false;
    return bud->plugin->plugin_table_len;
}
