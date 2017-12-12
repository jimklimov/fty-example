/*  =========================================================================
    fty_example_server - Actor

    Copyright (C) 2014 - 2017 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

/*
@header
    fty_example_server - Actor
@discuss
@end
*/

#include "fty_example_classes.h"

static void
s_handle_mailbox (mlm_client_t *client, zmsg_t **message_p)
{
    assert (client);
    assert (message_p);
    if (*message_p == NULL)
        return;
    zmsg_t *message = *message_p;

    char *part = zmsg_popstr (message);
    if (!part) {
        zmsg_destroy (&message);
        return;
    }
    if (streq (part, "HELLO")) {
        zmsg_t *reply  = zmsg_new ();
        zmsg_addstr (reply, "WORLD");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy (&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        } 
    }else{
        zmsg_t *reply  = zmsg_new ();
        zmsg_addstr (reply, "ERROR");

        int rv = mlm_client_sendto (client, mlm_client_sender (client), "example", NULL, 1000, &reply);
        if (rv != 0) {
            zmsg_destroy (&reply);
            zsys_error (
                    "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '1000') failed.",
                    mlm_client_sender (client), "example");
        }
    }
    zstr_free (&part);
    zmsg_destroy (&message);
}

static void
s_handle_stream (mlm_client_t *client, zmsg_t **message_p)
{
    assert (client);
    assert (message_p);
    if (*message_p == NULL)
        return;

    zmsg_t *message = *message_p;
    // On malamute streams we should receive only fty_proto protocol messages
    if (!is_fty_proto (message)) {
        zsys_error ("Received message is not fty_proto protocol message.");
        zmsg_destroy (message_p);
        return;
    }

    fty_proto_t *protocol_message = fty_proto_decode (&message);
    if (protocol_message == NULL) {
        zsys_error ("fty_proto_decode () failed. Received message could not be parsed.");
        return;
    }
    // Since we are subscribed to FTY_PROTO_STREAM_ASSETS,
    // received message should be FTY_PROTO_ASSET message 
    if (fty_proto_id (protocol_message) != FTY_PROTO_ASSET) {
        zsys_error (
                "Received message is not expected FTY_PROTO_ASSET id, but: '%d'.",
                fty_proto_id (protocol_message));
        fty_proto_destroy (&protocol_message);
        return;
    }

    uint64_t time = zclock_mono () / 1000;
    // We would like to demonstrate how to publish messages by sending an 'alert'
    // to FTY_PROTO_STREAM_ALERTS each time asset message is received -> Therefore 
    zlist_t *actions = zlist_new ();
    zlist_append (actions, "action");
    zmsg_t *reply_message = fty_proto_encode_alert (
            NULL,
            time,
            300,
            "example",
            fty_proto_name (protocol_message),
            "status",
            "severity",
            "example of how to publish a message",
            actions);
    fty_proto_destroy (&protocol_message);

    int rv = mlm_client_send (client, "example", &reply_message);
    if (rv != 0) {
        zmsg_destroy (&reply_message);
        zsys_error ("mlm_client_send (subject = '%s') failed");
        return;
    }
    zlist_destroy (&actions);
}

//  --------------------------------------------------------------------------
//  Actor function

void
fty_example_server (zsock_t *pipe, void* args)
{
    const char *endpoint = (const char *) args;
    zsys_debug ("endpoint: %s", endpoint);

    mlm_client_t *client = mlm_client_new ();
    if (client == NULL) {
        zsys_error ("mlm_client_new () failed.");
        return;
    }

    int rv = mlm_client_connect (client, endpoint, 1000, "fty-example");
    if (rv == -1) {
        mlm_client_destroy (&client);
        zsys_error (
                "mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed.",
                endpoint, 1000, "fty-example");
        return;
    } 

    rv = mlm_client_set_consumer (client, FTY_PROTO_STREAM_ASSETS, ".*");
    if (rv == -1) {
        mlm_client_destroy (&client);
        zsys_error (
                "mlm_client_set_consumer (stream = '%s', pattern = '%s') failed.",
                FTY_PROTO_STREAM_ASSETS, ".*");
        return;
    }

    rv = mlm_client_set_producer (client, FTY_PROTO_STREAM_ALERTS);
    if (rv == -1) {
        mlm_client_destroy (&client);
        zsys_error ("mlm_client_set_consumer (stream = '%s') failed.", FTY_PROTO_STREAM_ALERTS);
        return;
    } 

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (client), NULL);
    zsock_signal (pipe, 0);
    zsys_debug ("actor ready");

    while (!zsys_interrupted) {

        void *which = zpoller_wait (poller, -1);
        if (which == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            char *actor_command = zmsg_popstr (message);
            //  $TERM actor command implementation is required by zactor_t interface
            if (streq (actor_command, "$TERM")) {
                zstr_free (&actor_command);
                zmsg_destroy (&message);
                break;
            }
            zstr_free (&actor_command);
            zmsg_destroy (&message);
            continue;
        }

        zmsg_t *message = mlm_client_recv (client);
        if (message == NULL) {
            zsys_debug ("interrupted");
            break;
        }
        if (streq (mlm_client_command (client), "MAILBOX DELIVER")) {
            s_handle_mailbox (client, &message);
        }
        else
        if (streq (mlm_client_command (client), "STREAM DELIVER")) {
            s_handle_stream (client, &message);
        }
        else {
            zsys_warning (
                    "Unknown malamute pattern: '%s'. Message subject: '%s', sender: '%s'.",
                    mlm_client_command (client), mlm_client_subject (client), mlm_client_sender (client));
            zmsg_destroy (&message);
        }
    }

    mlm_client_destroy (&client);
    zpoller_destroy (&poller);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_example_server_test (bool verbose)
{
    printf (" * fty_example_server: ");

    //  @selftest
    static const char* endpoint = "inproc://fty-example-server-test";      

    // Note: If your selftest reads SCMed fixture data, please keep it in
    // src/selftest-ro; if your test creates filesystem objects, please
    // do so under src/selftest-rw. They are defined below along with a
    // usecase for the variables (assert) to make compilers happy.
    const char *SELFTEST_DIR_RO = "src/selftest-ro";
    const char *SELFTEST_DIR_RW = "src/selftest-rw";
    assert (SELFTEST_DIR_RO);
    assert (SELFTEST_DIR_RW);
    // Uncomment these to use C++ strings in C++ selftest code:
    //std::string str_SELFTEST_DIR_RO = std::string(SELFTEST_DIR_RO);
    //std::string str_SELFTEST_DIR_RW = std::string(SELFTEST_DIR_RW);
    //assert ( (str_SELFTEST_DIR_RO != "") );
    //assert ( (str_SELFTEST_DIR_RW != "") );
    // NOTE that for "char*" context you need (str_SELFTEST_DIR_RO + "/myfilename").c_str()

    //  Set up broker, fty example server actor and third party actor
    zactor_t *server = zactor_new (mlm_server, "Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose)
        zstr_send (server, "VERBOSE");

    zactor_t *example_server = zactor_new (fty_example_server, (void *) endpoint);

    //  Producer on FTY_PROTO_STREAM_ASSETS stream
    mlm_client_t *assets_producer = mlm_client_new ();
    int rv = mlm_client_connect (assets_producer, endpoint, 1000, "assets_producer");
    assert (rv == 0);
    rv = mlm_client_set_producer (assets_producer, FTY_PROTO_STREAM_ASSETS);
    assert (rv == 0);

    //  Consumer on FTY_PROTO_STREAM_ALERTS stream
    mlm_client_t *alerts_consumer = mlm_client_new ();
    rv = mlm_client_connect (alerts_consumer, endpoint, 1000, "alerts_consumer");
    assert (rv == 0);
    rv = mlm_client_set_consumer (alerts_consumer, FTY_PROTO_STREAM_ALERTS, ".*");
    assert (rv == 0);

    //  Test STREAM deliver:
    //  Create asset message and publish it on FTY_PROTO_STREAM_ASSETS stream
    zmsg_t *asset_message = fty_proto_encode_asset (NULL, "new asset 1", "create", NULL);
    assert (asset_message);
    rv = mlm_client_send (assets_producer, "example", &asset_message);
    assert (rv == 0);
    zmsg_t *message = mlm_client_recv (alerts_consumer);
    assert (message);
    //  zmsg_t needs to be decoded into fty_proto_t
    assert (is_fty_proto (message));
    fty_proto_t *alert_message = fty_proto_decode (&message);
    assert (fty_proto_id (alert_message) == FTY_PROTO_ALERT);
    //  Test that the alert published contains element_src equal to name of published asset
    assert (streq (fty_proto_name (alert_message), "new asset 1"));
    fty_proto_destroy (&alert_message);

    //  Test MAILBOX deliver:
    message = zmsg_new ();
    rv = mlm_client_sendto (alerts_consumer, "fty-example", "example", NULL, 1000, &message);
    assert (rv == 0);
    
    message = zmsg_new ();
    zmsg_addstr (message, "WRONG");
    rv = mlm_client_sendto (alerts_consumer, "fty-example", "example", NULL, 1000, &message);
    assert (rv == 0);
    message = mlm_client_recv (alerts_consumer);
    assert (message);
    char *error = zmsg_popstr (message);
    assert (error!=NULL);
    assert (streq (error, "ERROR"));
    zstr_free (&error);
    zmsg_destroy (&message);

    message = zmsg_new ();
    zmsg_addstr (message, "HELLO");
    rv = mlm_client_sendto (alerts_consumer, "fty-example", "example", NULL, 1000, &message);
    message = mlm_client_recv (alerts_consumer);
    assert (message);
    char *part = zmsg_popstr (message);
    assert (streq (part, "WORLD"));
    zstr_free (&part);
    zmsg_destroy (&message);

    mlm_client_destroy (&assets_producer);
    mlm_client_destroy (&alerts_consumer);
    zactor_destroy (&example_server);
    zactor_destroy (&server);
    
    //  @end
    printf ("OK\n");
}
