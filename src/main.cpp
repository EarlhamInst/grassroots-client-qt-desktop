/*
 ** Copyright 2014-2016 The Earlham Institute
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>


#undef _CRTDBG_MAP_ALLOC

#ifdef LINUX
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#endif

#include <string.h>
#include <sys/types.h>


#include "client.h"
#include "connection.h"
#include "string_utils.h"
#include "streams.h"
#include "client_ui_api.h"
#include "qt_client_data.h"
#include "curl_tools.h"

//#include "globus_auth.hpp"


#ifdef _DEBUG
#define STANDALONE_CLIENT_DEBUG	(STM_LEVEL_FINER)
#else
#define STANDALONE_CLIENT_DEBUG	(STM_LEVEL_NONE)
#endif


/*************************************/
/******* FUNCTION DEFINITIONS  *******/
/*************************************/

int main (int argc, char *argv [])
{
	if (argc < 3)
		{
			printf (
					"USAGE: grassroots-qt-client "
					"\t-h <server_url>, the web address of the Grassroots server to connect to.\n"
					"\t--list-all, get all available services from the Grassroots server.\n"
					"\t--get-service <service name> get named services from the Grassroots server.\n"
					"\t--list-interested <resource>, get all services that are able to run against a given resource.\n"
					"\t--get-service-data <service_name>, get the JSON for Lucene indexing.\n"
					"\t--verbose, display more information whilst running the client\n"
					"\t--username, username"
					"\t--password, password"
					"\t--globus_auth, authenticate via the Globus servers"
					"\t\tThe resource is in the form <protocol>://<name> e.g. file:///home/test.fa, https://my.data/object, irods://data.fa\n"
					);
			return 0;
		}
	else
		{
			int i = 1;
			const char *hostname_s = "localhost";
			const char *username_s = NULL;
			const char *password_s = NULL;
			char *protocol_s = NULL;
			char *query_s = NULL;
			const char *keyword_s = NULL;
			Operation op = OP_NONE;
			bool web_server_flag = true;
			CURLcode c;
			Connection *connection_p = NULL;
			bool verbose_flag = false;
			bool globus_auth_flag = false;
			const uint16 port = 1337;
			char *access_token_s = NULL;

			while (i < argc)
				{
					if (strcmp (argv [i], "-h") == 0)
						{
							if ((i + 1) < argc)
								{
									hostname_s = argv [++ i];
								}
							else
								{
									printf ("Hostname argument missing");
								}

						}
					else if (strcmp (argv [i], "--list-all") == 0)
						{
							op = OP_LIST_ALL_SERVICES;
						}
					else if (strcmp (argv [i], "--list-interested") == 0)
						{
							if ((i + 1) < argc)
								{
									const char *arg_s = argv [++ i];
									const char *delimiter_s = "://";
									const char *ptr = strstr (arg_s, delimiter_s);

									if (ptr)
										{
											protocol_s = CopyToNewString (arg_s, ptr - arg_s, false);

											if (protocol_s)
												{
													query_s = EasyCopyToNewString (ptr + strlen (delimiter_s));

													if (query_s)
														{
															op = OP_LIST_INTERESTED_SERVICES;
														}
													else
														{
															printf ("Failed to get resource name from \"%s\"", arg_s);
														}
												}
											else
												{
													printf ("Failed to copy resource type from \"%s\"", arg_s);
												}
										}
									else
										{
											printf ("Failed to find resource separator \"%s\" in \"%s\"", delimiter_s, arg_s);
										}

								}
							else
								{
									printf ("hostname argument missing");
								}
						}
					else if (strcmp (argv [i], "--get-service") == 0)
						{
							op = OP_GET_NAMED_SERVICES;

							if ((i + 1) < argc)
								{
									keyword_s = argv [++ i];
								}
							else
								{
									printf ("named service argument missing");
								}
						}
					else if (strcmp (argv [i], "--get-service-data") == 0)
						{
							op = OP_GET_SERVICE_INFO;

							if ((i + 1) < argc)
								{
									keyword_s = argv [++ i];
								}
							else
								{
									printf ("named service argument missing");
								}
						}
					else if (strcmp (argv [i], "--username") == 0)
						{
							if ((i + 1) < argc)
								{
									username_s = argv [++ i];
								}
							else
								{
									printf ("username argument missing");
								}
						}
					else if (strcmp (argv [i], "--password") == 0)
						{
							if ((i + 1) < argc)
								{
									password_s = argv [++ i];
								}
							else
								{
									printf ("password argument missing");
								}
						}
					else if (strcmp (argv [i], "--verbose") == 0)
						{
							verbose_flag = true;
						}
					else if (strcmp (argv [i], "--globus_auth") == 0)
						{
							globus_auth_flag = true;
						}
					else
						{
							printf ("Unknown argument: \"%s\"", argv [i]);
						}

					++ i;
				}		/* while (i < argc) */


			c = curl_global_init (CURL_GLOBAL_DEFAULT);

			if (c == 0)
				{
					connection_p = AllocateWebServerConnection (hostname_s, CM_MEMORY);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to initialise curl environment: %d", c);
				}


			if (connection_p)
				{
					bool success_flag = false;

					if (globus_auth_flag)
						{
							static char *app_s = EasyCopyToNewString ("Grassroots Auth");

							/*
							//QStyle *style_p = QStyleFactory :: create ("Fusion");
							// /QApplication :: setStyle (style_p);

							//qDebug() << QStyleFactory::keys();

							QApplication *app_p = new QApplication (argc, argv);

							QWindow *win_p = new QWindow;

							win_p -> setTitle (app_s);
							win_p -> show ();


							GlobusAuth *auth_p = new GlobusAuth (win_p, username_s, password_s, port);

							auth_p -> Authenticate ();
							app_p -> exec ();

							if (auth_p -> IsAccessTokenSet ())
								{
									const char *token_s = auth_p -> GetAccessToken ();

									access_token_s = EasyCopyToNewString (token_s);

									if (!access_token_s)
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy access token");
										}
								}

							if (AddConnectionHeader (connection_p, "token_type", "Bearer"))
								{
									if (AddConnectionHeader (connection_p, "access_token", access_token_s))
										{
											success_flag = true;
										}

								}

								*/
							success_flag = true;
						}
					else
						{
							success_flag = true;
						}

					if (success_flag)
						{
							if (InitDefaultOutputStream ())
								{
									Client *client_p = GetClient (connection_p, argc, argv);

									if (client_p)
										{
											QTClientData *qt_data_p = reinterpret_cast <QTClientData *> (client_p -> cl_data_p);
											SchemaVersion *sv_p = AllocateSchemaVersion (CURRENT_SCHEMA_VERSION_MAJOR, CURRENT_SCHEMA_VERSION_MINOR);
											User *user_p = NULL;

											if (!globus_auth_flag)
												{
													if (username_s && password_s)
														{
															if (!SetConnectionCredentials (connection_p, username_s, password_s))
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set credentials");
																}
														}
												}

											SetClientSchema (client_p, sv_p);

											qt_data_p -> qcd_verbose_flag = verbose_flag;

											if (connection_p -> co_type == CT_WEB)
												{
													SetCurlToolVerbose (((WebConnection *) connection_p) -> wc_curl_p, verbose_flag);
												}

											switch (op)
												{
													case OP_LIST_ALL_SERVICES:
														{
															GetAllServicesInClient (client_p, user_p);
														}
														break;

													case OP_LIST_INTERESTED_SERVICES:
														{
															GetInterestedServicesInClient (client_p, protocol_s, query_s, user_p);
														}
														break;

													case OP_GET_NAMED_SERVICES:
														{
															GetNamedServicesInClient (client_p, keyword_s, user_p);
														}
														break;


													case OP_GET_SERVICE_INFO:
														{
															GetNamedServicesIndexingDataInClient (client_p, keyword_s, user_p);
														}
														break;

													default:
														break;
												}		/* switch (api_id) */

											ReleaseClient (client_p);
										}		/* if (client_p) */


									FreeDefaultOutputStream ();
								}

						}		/*if (success_flag) */


					FreeConnection (connection_p);

					if (web_server_flag)
						{
							curl_global_cleanup ();
						}
				}
			else
				{
					printf ("Failed to connect to server %s\n", hostname_s);
				}

			if (query_s)
				{
					FreeCopiedString (query_s);
				}

			if (protocol_s)
				{
					FreeCopiedString (protocol_s);
				}

			if (access_token_s)
				{
					FreeCopiedString (access_token_s);
				}

		}


	return 0;
}

