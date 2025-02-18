/*
 * Authors:
 * Dmitriy A. Gearasimov <kahovski@gmail.com>
 * DeM Labs Inc.   https://demlabs.net
 * DeM Labs Open source community https://github.com/demlabsinc
 * Copyright  (c) 2017-2019
 * All rights reserved.

 This file is part of DAP (Deus Applications Prototypes) the open source project

    DAP (Deus Applicaions Prototypes) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DAP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with any DAP based project.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <io.h>
#endif

#include <pthread.h>

#include "dap_common.h"

#include "sig_unix_handler.h"

#include "dap_config.h"
#include "dap_server.h"
#include "dap_http.h"
#include "dap_http_folder.h"
#include "dap_events.h"
#include "dap_enc.h"
#include "dap_enc_ks.h"
#include "dap_enc_http.h"
#include "dap_chain.h"
#include "dap_chain_wallet.h"

#include "dap_cert.h"
#include "dap_cert_file.h"

#include "dap_chain_cs_dag.h"
#include "dap_chain_cs_dag_event.h"
#include "dap_chain_cs_dag_poa.h"
#include "dap_chain_cs_dag_pos.h"

#include "dap_chain_net.h"
#include "dap_chain_net_srv.h"
#include "dap_chain_net_srv_app.h"
#include "dap_chain_net_srv_app_db.h"
#include "dap_chain_net_srv_datum.h"

#ifdef DAP_OS_LINUX
#include "dap_chain_net_srv_vpn.h"
#endif

#include "dap_stream_session.h"
#include "dap_stream.h"
#include "dap_stream_ch_chain.h"
#include "dap_stream_ch_chain_net.h"
#include "dap_stream_ch_chain_net_srv.h"

#include "dap_chain_wallet.h"

#include "dap_client.h"
#include "dap_http_simple.h"
#include "dap_process_manager.h"
#include "dap_defines.h"
#ifdef _WIN32
#include "registry.h"
#endif

#define ENC_HTTP_URL "/enc_init"
#define STREAM_CTL_URL "/stream_ctl"
#define STREAM_URL "/stream"
#define SLIST_URL "/nodelist"
#define MAIN_URL "/"
#define LOG_TAG "main_node_tool"

#ifdef __ANDROID__
    #include "cellframe_node.h"
#endif

#undef log_it
#ifdef DAP_OS_WINDOWS
#define log_it(_log_level, string, ...) __mingw_printf(string, ##__VA_ARGS__)
#else
#define log_it(_log_level, string, ...) printf(string, ##__VA_ARGS__)
#endif

static int s_init( int argc, const char * argv[] );
static void s_help( );

static char s_system_ca_dir[MAX_PATH];
static char s_system_wallet_dir[MAX_PATH];

#ifdef __ANDROID__
int cellframe_node_tool_Main(int argc, const char **argv)
#else

int main(int argc, const char **argv)
#endif
{
  int ret = s_init( argc, argv );

  if ( ret ) {
    log_it( L_ERROR, "Can't init modules" );
    return ret;
  }

  if ( argc < 2 ) {
    log_it( L_INFO, "No params. Nothing to do" );
    s_help( );
    exit( -1000 );
  }

  if ( strcmp ( argv[1], "wallet" ) == 0 ) {

    if ( argc < 3 ) {
      log_it(L_ERROR,"Wrong 'wallet' command params");
      s_help();
      exit(-2001);
    }

    if ( strcmp( argv[2],"create") == 0 ) {

      // wallet create <network name> <wallet name> <wallet_sign>
      if ( argc < 5 ) {
        log_it( L_ERROR, "Wrong 'wallet create' command params" );
        s_help( );
        exit( -2003 );
      }

      const char *l_wallet_name = argv[3];
      dap_sign_type_t l_sig_type = dap_sign_type_from_str( argv[4] );
      dap_chain_wallet_t *l_wallet = NULL;

      //
      // Check if wallet name has only digits and English letters
      //

      size_t is_str = dap_isstralnum(l_wallet_name);

      if (!is_str)
      {
          log_it( L_ERROR, "Wallet name must contain digits and alphabet symbols");
          exit( -2004 );
      }

      if ( l_sig_type.type == SIG_TYPE_NULL ) {
        log_it( L_ERROR, "Wrong signature '%s'", argv[4] );
        s_help( );
        exit( -2004 );
      }

      //
      // Check unsupported tesla algorithm
      //

      if (l_sig_type.type == SIG_TYPE_TESLA)
      {
          log_it( L_ERROR, "Tesla algorithm is not supported, please, use another variant");
          exit( -2004 );
      }

      l_wallet = dap_chain_wallet_create(l_wallet_name, s_system_wallet_dir, l_sig_type);
    }
    else if ( strcmp( argv[2],"sign_file") == 0 ) {
      // wallet sign_file <wallet name> <cert index> <data file path> <data offset> <data length> <dsign file path>
      if ( argc < 8 ) {
        log_it(L_ERROR,"Wrong 'wallet sign_file' command params");
        s_help();
        exit(-3000);
      }
      dap_chain_wallet_t *l_wallet = dap_chain_wallet_open(argv[3], s_system_wallet_dir);
      if ( !l_wallet ) {
        log_it(L_ERROR,"Can't open wallet \"%s\"",argv[3]);
        s_help();
        exit(-3001);
      }

      int l_cert_index = atoi(argv[4]);

      size_t l_wallet_certs_number = dap_chain_wallet_get_certs_number( l_wallet );
      if ( (l_cert_index > 0) && (l_wallet_certs_number > (size_t)l_cert_index) ) {
        FILE *l_data_file = fopen( argv[5],"rb" );
        if ( l_data_file ) {}
      }
      else {
        log_it( L_ERROR, "Cert index %d can't be found in wallet with %zu certs inside"
                                           ,l_cert_index,l_wallet_certs_number );
        s_help();
        exit( -3002 );
      }
    }
  } // wallet
  else if (strcmp (argv[1],"cert") == 0 ) {
    if ( argc >=3 ) {
      if ( strcmp( argv[2],"dump") == 0 ){
        if (argc>=4) {
          const char * l_cert_name = argv[3];
          dap_cert_t * l_cert = dap_cert_add_file(l_cert_name, s_system_ca_dir);
          if ( l_cert ) {
            dap_cert_dump(l_cert);
            dap_cert_delete_by_name(l_cert_name);
            ret = 0;
          }
          else {
            exit(-702);
          }
        }
     } else if ( strcmp( argv[2],"create_pkey") == 0 ){
       if (argc < 5) exit(-7023);
         const char *l_cert_name = argv[3];
         const char *l_cert_pkey_path = argv[4];
         dap_cert_t *l_cert = dap_cert_add_file(l_cert_name, s_system_ca_dir);
         if ( !l_cert ) exit( -7021 );
           l_cert->enc_key->pub_key_data_size = dap_enc_gen_key_public_size(l_cert->enc_key);
           if ( l_cert->enc_key->pub_key_data_size ) {
             //l_cert->key_private->pub_key_data = DAP_NEW_SIZE(void, l_cert->key_private->pub_key_data_size);
             //if ( dap_enc_gen_key_public(l_cert->key_private, l_cert->key_private->pub_key_data) == 0){
             dap_pkey_t * l_pkey = dap_pkey_from_enc_key( l_cert->enc_key );
             if (l_pkey) {
               FILE *l_file = fopen(l_cert_pkey_path,"wb");
               if (l_file) {
                 fwrite(l_pkey,1,l_pkey->header.size + sizeof(l_pkey->header),l_file);
                 fclose(l_file);
               }
             } else {
               log_it(L_ERROR, "Can't produce pkey from the certificate");
               exit(-7022);
             }
             dap_cert_delete_by_name(l_cert_name);
             ret = 0;
             //}else{
             //    log_it(L_ERROR,"Can't produce public key with this key type");
             //    exit(-7024);
             //}
           } else {
             log_it(L_ERROR,"Can't produce pkey from this cert type");
             exit(-7023);
           }
     } else if ( strcmp( argv[2],"create_cert_pkey") == 0 ) {
       if ( argc >= 5 ) {
         const char *l_cert_name = argv[3];
         const char *l_cert_new_name = argv[4];
         dap_cert_t *l_cert = dap_cert_add_file(l_cert_name, s_system_ca_dir);
         if ( l_cert ) {
           if ( l_cert->enc_key->pub_key_data_size ) {
             // Create empty new cert
             dap_cert_t * l_cert_new = dap_cert_new(l_cert_new_name);
             l_cert_new->enc_key = dap_enc_key_new( l_cert->enc_key->type);

             // Copy only public key pointer
             l_cert_new->enc_key->pub_key_data = DAP_NEW_Z_SIZE(uint8_t,
                                                                l_cert_new->enc_key->pub_key_data_size =
                                                                l_cert->enc_key->pub_key_data_size );
             memcpy(l_cert_new->enc_key->pub_key_data, l_cert->enc_key->pub_key_data,l_cert->enc_key->pub_key_data_size);

             dap_cert_save_to_folder(l_cert_new, s_system_ca_dir);
           } else {
             log_it(L_ERROR,"Can't produce pkey from this cert type");
             exit(-7023);
           }
         } else {
           exit(-7021);
         }
       }
     } else if ( strcmp( argv[2],"create" ) == 0 ) {
       if ( argc < 5 ) {
         s_help();
         exit(-500);
       }
       const char *l_cert_name = argv[3];
       size_t l_cert_path_length = strlen(argv[3])+8+strlen(s_system_ca_dir);
       char *l_cert_path = DAP_NEW_Z_SIZE(char,l_cert_path_length);
       snprintf(l_cert_path,l_cert_path_length,"%s/%s.dcert",s_system_ca_dir,l_cert_name);
       if ( access( l_cert_path, F_OK ) != -1 ) {
         log_it (L_ERROR, "File %s is already exists! Who knows, may be its smth important?", l_cert_path);
         exit(-700);
       }

       dap_enc_key_type_t l_key_type = DAP_ENC_KEY_TYPE_NULL;

       // Check unsupported tesla algorithm
       if (dap_strcmp (argv[4],"sig_tesla") == 0){
          log_it( L_ERROR, "Tesla algorithm is not supported, please, use another variant");
          exit(-600);
       }

       //Check if certificate name has only digits and English letters
       if (!dap_isstralnum(l_cert_name)){
          log_it( L_ERROR, "Certificate name must contains digits and alphabet symbols");
          exit( -2004 );
       }

       if ( dap_strcmp (argv[4],"sig_bliss") == 0 ){
         l_key_type = DAP_ENC_KEY_TYPE_SIG_BLISS;
       } else if ( dap_strcmp (argv[4],"sig_picnic") == 0){
         l_key_type = DAP_ENC_KEY_TYPE_SIG_PICNIC;
       } else if ( dap_strcmp(argv[4],"sig_dil") == 0){
        l_key_type = DAP_ENC_KEY_TYPE_SIG_DILITHIUM;
       }
       else {
         log_it (L_ERROR, "Wrong key create action \"%s\"",argv[4]);
         exit(-600);
       }

       if ( l_key_type != DAP_ENC_KEY_TYPE_NULL ) {
         dap_cert_t * l_cert = dap_cert_generate(l_cert_name,l_cert_path,l_key_type ); // key length ignored!
         if (l_cert == NULL){
           log_it(L_ERROR, "Can't create %s",l_cert_path);
         }
         dap_cert_delete(l_cert);
       } else {
           s_help();
           DAP_DELETE(l_cert_path);
           exit(-500);
       }
       DAP_DELETE(l_cert_path);
     } else if (strcmp(argv[2], "add_metadata") == 0) {
       if (argc >= 5) {
         const char *l_cert_name = argv[3];
         dap_cert_t *l_cert = dap_cert_add_file(l_cert_name, s_system_ca_dir);
         if ( l_cert ) {
           char **l_params = dap_strsplit(argv[4], ":", 4);
           dap_cert_metadata_type_t l_type = (dap_cert_metadata_type_t)atoi(l_params[1]);
           if (l_type == DAP_CERT_META_STRING || l_type == DAP_CERT_META_SIGN || l_type == DAP_CERT_META_CUSTOM) {
             dap_cert_add_meta(l_cert, l_params[0], l_type, (void *)l_params[3], strtoul(l_params[2], NULL, 10));
           } else {
             dap_cert_add_meta_scalar(l_cert, l_params[0], l_type,
                                      strtoull(l_params[3], NULL, 10), strtoul(l_params[2], NULL, 10));
           }
           dap_strfreev(l_params);
           dap_cert_save_to_folder(l_cert, s_system_ca_dir);
           dap_cert_delete_by_name(l_cert_name);
           ret = 0;
         }
         else {
           exit(-800);
         }
       }
     } else {
       log_it(L_ERROR,"Wrong params");
       s_help();
       exit(-1000);
     }
   } else {
     log_it(L_ERROR,"Wrong params");
     s_help();
     exit(-1000);
   }
 }else {
   log_it(L_ERROR,"Wrong params");
   s_help();
   exit(-1000);
 }

}

/**
 * @brief s_init
 * @param argc
 * @param argv
 * @return
 */
static int s_init( int argc, const char **argv )
{
    UNUSED(argc);
    UNUSED(argv);
    dap_set_appname("cellframe-node");
#ifdef _WIN32
    g_sys_dir_path = dap_strdup_printf("%s/%s", regGetUsrPath(), dap_get_appname());
    char * s_log_dir_path = dap_strdup_printf("%s/var/log", g_sys_dir_path) ;
#elif DAP_OS_MAC
    char * l_username = NULL;
    exec_with_ret(&l_username,"whoami|tr -d '\n'");
    if (!l_username){
        printf("Fatal Error: Can't obtain username");
    return 2;
    }
    g_sys_dir_path = dap_strdup_printf("/Users/%s/Applications/Cellframe.app/Contents/Resources", l_username);
    DAP_DELETE(l_username);
#elif DAP_OS_ANDROID
    g_sys_dir_path = dap_strdup_printf("/storage/emulated/0/opt/%s",dap_get_appname());
    char * s_log_dir_path = dap_strdup_printf("%s/var/log", g_sys_dir_path) ;
#elif DAP_OS_UNIX
    g_sys_dir_path = dap_strdup_printf("/opt/%s", dap_get_appname());
    char * s_log_dir_path = dap_strdup_printf("%s/var/log", g_sys_dir_path) ;
#endif
    char l_config_dir[MAX_PATH] = {'\0'};
    dap_sprintf(l_config_dir, "%s/etc", g_sys_dir_path);
    dap_config_init(l_config_dir);
    g_config = dap_config_open(dap_get_appname());
    if (g_config) {
        uint16_t l_ca_folders_size = 0;
        char **l_ca_folders = dap_config_get_array_str(g_config, "resources", "ca_folders", &l_ca_folders_size);
        dap_stpcpy(s_system_ca_dir, l_ca_folders[0]);
        const char *l_wallet_folder = dap_config_get_item_str(g_config, "resources", "wallets_path");
        dap_stpcpy(s_system_wallet_dir, l_wallet_folder);
    } else {
        dap_stpcpy(s_system_ca_dir, "./");
        dap_stpcpy(s_system_wallet_dir, "./");
    }
    return 0;
}

/**
 * @brief s_help
 * @param a_appname
 */
static void s_help()
{
#ifdef _WIN32
    SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), 7 );
#endif
    char *l_tool_appname = dap_strdup_printf("%s-tool", dap_get_appname());
    printf( "\n" );
    printf( "%s usage:\n\n", l_tool_appname);

    printf(" * Create new key wallet and generate signatures with same names plus index \n" );
    printf("\t%s wallet create <network name> <wallet name> <signature type> [<signature type 2>[...<signature type N>]]\n\n", l_tool_appname);

    printf(" * Create new key wallet from existent certificates in the system\n");
    printf("\t%s wallet create_from <network name> <wallet name> <wallet ca1> [<wallet ca2> [...<wallet caN>]]\n\n", l_tool_appname);

    printf(" * Create new key file with randomly produced key stored in\n");
    printf("\t%s cert create <cert name> <key type> [<key length>]\n\n", l_tool_appname);

    printf(" * Dump cert data stored in <file path>\n");
    printf("\t%s cert dump <cert name>\n\n", l_tool_appname);

    printf(" * Sign some data with cert \n");
    printf("\t%s cert sign <cert name> <data file path> <sign file output> [<sign data length>] [<sign data offset>]\n\n", l_tool_appname);

    printf(" * Create pkey from <cert name> and store it on <pkey path>\n");
    printf("\t%s cert create_pkey <cert name> <pkey path>\n\n", l_tool_appname);

    printf(" * Export only public key from <cert name> and stores it \n");
    printf("\t%s cert create_cert_pkey <cert name> <new cert name>\n\n", l_tool_appname);

    printf(" * Add metadata item to <cert name>\n");
    printf("\t%s cert add_metadata <cert name> <key:type:length:value>\n\n", l_tool_appname);
}
