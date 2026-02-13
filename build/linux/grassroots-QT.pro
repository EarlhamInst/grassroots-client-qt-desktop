#-------------------------------------------------
#
# Project created by QtCreator 2014-09-11T12:08:32
#
#-------------------------------------------------

QT       += core gui networkauth

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webenginewidgets

TARGET = grassroots-desktop-client
TEMPLATE = app

VPATH += src include

unix {
 CONFIG(release, debug|release) {
	BUILD = release
 }
 CONFIG(debug, debug|release) {
	BUILD = debug
	DEFINES += "_DEBUG"
 }
}


# Include the user preferemces
include (dependencies.pri)

DIR_GRASSROOTS_QT_SRC = ../../

# Path to the Grassroots libraries
DIR_GRASSROOTS_LIBS = $$DIR_GRASSROOTS_INSTALL_ROOT/lib

DIR_GRASSROOTS_SHARED_SRC = $$DIR_GRASSROOTS_CORE/shared
DIR_GRASSROOTS_SERVER_SRC = $$DIR_GRASSROOTS_CORE/server

#DIR_GRASSROOTS_EXTRAS = $$DIR_GRASSROOTS_INSTALL_ROOT/extras

DIR_GRASSROOTS_CLIENTS = $$DIR_GRASSROOTS_CORE/client
DIR_GRASSROOTS_CLIENTS_INC = $$DIR_GRASSROOTS_CLIENTS/include
GRASSROOTS_CLIENTS_LIB_NAME = grassroots_clients

DIR_GRASSROOTS_UTIL = $$DIR_GRASSROOTS_SHARED_SRC/util
DIR_GRASSROOTS_UTIL_INC = $$DIR_GRASSROOTS_UTIL/include
GRASSROOTS_UTIL_LIB_NAME = grassroots_util

DIR_GRASSROOTS_USERS = $$DIR_GRASSROOTS_SHARED_SRC/users
DIR_GRASSROOTS_USERS_INC = $$DIR_GRASSROOTS_USERS/include
GRASSROOTS_USERS_LIB_NAME = grassroots_users


DIR_GRASSROOTS_UUID = $$DIR_GRASSROOTS_SHARED_SRC/uuid
DIR_GRASSROOTS_UUID_INC = $$DIR_GRASSROOTS_UUID/include
GRASSROOTS_UUID_LIB_NAME = grassroots_uuid

DIR_GRASSROOTS_SERVER = $$DIR_GRASSROOTS_SERVER_SRC/server
DIR_GRASSROOTS_SERVER_INC = $$DIR_GRASSROOTS_SERVER/include
GRASSROOTS_SERVER_LIB_NAME = grassroots_server


DIR_GRASSROOTS_PLUGIN = $$DIR_GRASSROOTS_SERVER_SRC/plugin
DIR_GRASSROOTS_PLUGIN_INC = $$DIR_GRASSROOTS_PLUGIN/include
GRASSROOTS_PLUGIN_LIB_NAME = grassroots_plugin


DIR_GRASSROOTS_TASK = $$DIR_GRASSROOTS_SERVER_SRC/task
DIR_GRASSROOTS_TASK_INC = $$DIR_GRASSROOTS_TASK/include
GRASSROOTS_TASK_LIB_NAME = grassroots_task


DIR_GRASSROOTS_DRMAA = $DIR_GRASSROOTS_SERVER_SRC/drmaa
GRASSROOTS_DRMAA_INC = $DIR_GRASSROOTS_DRMAA/include

DIR_GRASSROOTS_MONGODB = $$DIR_GRASSROOTS_SERVER_SRC/mongodb
DIR_GRASSROOTS_MONGODB_INC = $$DIR_GRASSROOTS_MONGODB/include
GRASSROOTS_MONGODB_LIB_NAME = grassroots_mongodb

DIR_GRASSROOTS_NETWORK = $$DIR_GRASSROOTS_SHARED_SRC/network
DIR_GRASSROOTS_NETWORK_INC = $$DIR_GRASSROOTS_NETWORK/include
GRASSROOTS_NETWORK_LIB_NAME = grassroots_network

DIR_GRASSROOTS_SERVICE = $$DIR_GRASSROOTS_SHARED_SRC/services
DIR_GRASSROOTS_SERVICE_INC = $$DIR_GRASSROOTS_SERVICE/include
GRASSROOTS_SERVICE_LIB_NAME = grassroots_service

DIR_GRASSROOTS_HANDLER = $$DIR_GRASSROOTS_SHARED_SRC/handler
DIR_GRASSROOTS_HANDLER_INC = $$DIR_GRASSROOTS_HANDLER/include
GRASSROOTS_HANDLER_LIB_NAME = grassroots_handler

DIR_GRASSROOTS_IRODS = $$DIR_GRASSROOTS_SHARED_SRC/handler
DIR_GRASSROOTS_IRODS_INC = $$DIR_GRASSROOTS_IRODS/include
GRASSROOTS_IRODS_LIB_NAME = grassroots_irods


DIR_GRASSROOTS_LIBUUID = $$DIR_GRASSROOTS_EXTRAS/libuuid
DIR_GRASSROOTS_LIBUUID_INC = $$DIR_GRASSROOTS_LIBUUID/include
DIR_GRASSROOTS_LIBUUID_LIB = $$DIR_GRASSROOTS_LIBUUID/lib
GRASSROOTS_LIBUUID_LIB_NAME = uuid

DIR_GRASSROOTS_PCRE2 = /home/billy/Applications/pcre2
DIR_GRASSROOTS_PCRE2_INC = $$DIR_GRASSROOTS_PCRE2/include
DIR_GRASSROOTS_PCRE2_LIB = $$DIR_GRASSROOTS_PCRE2/lib
GRASSROOTS_PCRE2_LIB_NAME = pcre2-8

DIR_GRASSROOTS_JANSSON = $$DIR_GRASSROOTS_EXTRAS/jansson
DIR_GRASSROOTS_JANSSON_INC = $$DIR_GRASSROOTS_JANSSON/include
DIR_GRASSROOTS_JANSSON_LIB = $$DIR_GRASSROOTS_JANSSON/lib
GRASSROOTS_JANSSON_LIB_NAME = jansson

DIR_GRASSROOTS_MONGOC = $$DIR_GRASSROOTS_EXTRAS/mongodb-c
DIR_GRASSROOTS_MONGOC_INC = $$DIR_GRASSROOTS_MONGOC/include/mongoc-2.2.2
DIR_GRASSROOTS_MONGOC_LIB = $$DIR_GRASSROOTS_MONGOC/lib
DIR_GRASSROOTS_BSON_INC = $$DIR_GRASSROOTS_MONGOC/include/bson-2.2.2
GRASSROOTS_MONGOC_LIB_NAME = mongoc2
GRASSROOTS_BSON_LIB_NAME = bson2


DIR_IRODS_LIBS = $$DIR_IRODS_HOME/usr/lib


DEFINES += "LINUX" "_DEBUG" "SHARED_LIBRARY" # "QT_NO_DEBUG_OUTPUT"

INCLUDEPATH += \
		$$DIR_GRASSROOTS_QT_SRC/include \
		$$DIR_GRASSROOTS_LIBUUID_INC \
		$$DIR_GRASSROOTS_JANSSON_INC \
		$$DIR_GRASSROOTS_CLIENTS_INC \
		$$DIR_GRASSROOTS_UUID_INC \
		$$DIR_GRASSROOTS_UTIL_INC \
		$$DIR_GRASSROOTS_UTIL_INC/containers \
		$$DIR_GRASSROOTS_UTIL_INC/io \
		$$DIR_GRASSROOTS_USERS_INC \
		$$DIR_GRASSROOTS_SERVICE_INC \
		$$DIR_GRASSROOTS_SERVICE_INC/parameters \
		$$DIR_GRASSROOTS_SERVER_INC \
		$$DIR_GRASSROOTS_NETWORK_INC \
		$$DIR_GRASSROOTS_HANDLER_INC \
		$$DIR_GRASSROOTS_TASK_INC \
		$$DIR_GRASSROOTS_PLUGIN_INC \
		$$DIR_GRASSROOTS_MONGOC_INC \
		$$DIR_GRASSROOTS_BSON_INC \



SOURCES += \
		$$DIR_GRASSROOTS_QT_SRC/src/base_param_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/client_ui_api.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/file_chooser_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/json_list_widget_item.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/json_viewer.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/keyword_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/main.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/main_window.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/matched_service_list_widget_item.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/param_check_box.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/param_double_spin_box.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/param_group_box.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/param_json_editor.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/param_line_edit.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/string_table_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/param_text_box.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/parameter_widget_container.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/prefs_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/progress_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/progress_window.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/qt_client_data.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/qt_parameter_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/repeatable_param_group_box.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/results_list.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/results_page.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/results_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/results_window.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/servce_ui.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/service_prefs_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/services_list.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/services_tabs.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/standard_list_widget_item.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/text_viewer.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/ui_utils.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/viewable_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/viewer_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/param_date_widget.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/progress_widget_delegate.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/double_spinner.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/signed_int_param_spin_box.cpp \
		$$DIR_GRASSROOTS_QT_SRC/src/unsigned_int_param_spin_box.cpp \
	../../src/alloc_failure.cpp \
	../../src/auth.cpp \
	../../src/base_combo_box.cpp \
	../../src/base_table_widget.cpp \
	../../src/double_combo_box.cpp \
	../../src/droppable_table_widget.cpp \
	../../src/droppable_text_box.cpp \
	../../src/globus_auth.cpp \
	../../src/json_table_widget.cpp \
	../../src/labels_widget.cpp \
	../../src/param_char_edit.cpp \
	../../src/signed_int_combo_box.cpp \
	../../src/signed_int_spinner.cpp \
	../../src/string_combo_box.cpp \
	../../src/string_list_widget.cpp \
	../../src/unsigned_int_combo_box.cpp \
	../../src/unsigned_int_validator.cpp \
	../../src/unsigned_int_spinner.cpp

HEADERS  += \
		$$DIR_GRASSROOTS_QT_SRC/include/base_param_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/client_ui_api.h \
		$$DIR_GRASSROOTS_QT_SRC/include/droppable_target_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/file_chooser_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/json_list_widget_item.h \
		$$DIR_GRASSROOTS_QT_SRC/include/json_viewer.h \
		$$DIR_GRASSROOTS_QT_SRC/include/keyword_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/main_window.h \
		$$DIR_GRASSROOTS_QT_SRC/include/matched_service_list_widget_item.h \
		$$DIR_GRASSROOTS_QT_SRC/include/param_check_box.h \
		$$DIR_GRASSROOTS_QT_SRC/include/param_double_spin_box.h \
		$$DIR_GRASSROOTS_QT_SRC/include/param_group_box.h \
		$$DIR_GRASSROOTS_QT_SRC/include/param_json_editor.h \
		$$DIR_GRASSROOTS_QT_SRC/include/param_line_edit.h \
		$$DIR_GRASSROOTS_QT_SRC/include/string_table_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/param_text_box.h \
		$$DIR_GRASSROOTS_QT_SRC/include/parameter_widget_container.h \
		$$DIR_GRASSROOTS_QT_SRC/include/prefs_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/progress_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/progress_window.h \
		$$DIR_GRASSROOTS_QT_SRC/include/qt_client_data.h \
		$$DIR_GRASSROOTS_QT_SRC/include/qt_parameter_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/repeatable_param_group_box.h \
		$$DIR_GRASSROOTS_QT_SRC/include/results_list.h \
		$$DIR_GRASSROOTS_QT_SRC/include/results_page.h \
		$$DIR_GRASSROOTS_QT_SRC/include/results_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/results_window.h \
		$$DIR_GRASSROOTS_QT_SRC/include/runnable_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/service_prefs_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/service_ui.h \
		$$DIR_GRASSROOTS_QT_SRC/include/services_list.h \
		$$DIR_GRASSROOTS_QT_SRC/include/services_tabs.h \
		$$DIR_GRASSROOTS_QT_SRC/include/standard_list_widget_item.h \
		$$DIR_GRASSROOTS_QT_SRC/include/text_viewer.h \
		$$DIR_GRASSROOTS_QT_SRC/include/ui_utils.h \
		$$DIR_GRASSROOTS_QT_SRC/include/viewable_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/viewer_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/param_date_widget.h \
		$$DIR_GRASSROOTS_QT_SRC/include/progress_widget_delegate.h \
		$$DIR_GRASSROOTS_QT_SRC/include/double_spinner.h \
		$$DIR_GRASSROOTS_QT_SRC/include/signed_int_param_spin_box.h \
		$$DIR_GRASSROOTS_QT_SRC/include/unsigned_int_param_spin_box.h \
	../../include/alloc_failure.hpp \
	../../include/auth.hpp \
	../../include/base_combo_box.h \
	../../include/base_table_widget.h \
	../../include/double_combo_box.h \
	../../include/droppable_table_widget.h \
	../../include/droppable_text_box.h \
	../../include/globus_auth.hpp \
	../../include/json_table_widget.h \
	../../include/labels_widget.h \
	../../include/param_char_edit.h \
	../../include/signed_int_combo_box.h \
	../../include/signed_int_spinner.h \
	../../include/string_combo_box.h \
	../../include/string_list_widget.h \
	../../include/unsigned_int_combo_box.h \
	../../include/unsigned_int_spinner.h \
	../../include/unsigned_int_validator.h


BOOST_DEPS =  -L$$DIR_IRODS_BOOST_LIBS \
 -lboost_chrono \
 -lboost_program_options \
 -lboost_regex \
 -lboost_thread \
 -lboost_filesystem \
 -lboost_system

DRMAA_DEPS = -L $$DIR_DRMAA_LIB \
 -ldrmaa


LIBS += -L$$DIR_GRASSROOTS_LIBS \
 -l$$GRASSROOTS_SERVICE_LIB_NAME \
 -l$$GRASSROOTS_MONGODB_LIB_NAME \
 -l$$GRASSROOTS_CLIENTS_LIB_NAME \
 -l$$GRASSROOTS_NETWORK_LIB_NAME \
 -l$$GRASSROOTS_HANDLER_LIB_NAME \
 -l$$GRASSROOTS_SERVER_LIB_NAME \
 -l$$GRASSROOTS_PLUGIN_LIB_NAME \
 -l$$GRASSROOTS_TASK_LIB_NAME \
 -l$$GRASSROOTS_UUID_LIB_NAME \
 -l$$GRASSROOTS_UTIL_LIB_NAME \
 -l$$GRASSROOTS_USERS_LIB_NAME \
 -L$$DIR_GRASSROOTS_JANSSON_LIB  -l$$GRASSROOTS_JANSSON_LIB_NAME \
 -L$$DIR_GRASSROOTS_MONGOC_LIB -l$$GRASSROOTS_MONGOC_LIB_NAME \
 -l$$GRASSROOTS_BSON_LIB_NAME \
 -L$$DIR_GRASSROOTS_LIBUUID_LIB \
 -lcurl \
 -lpthread \
 -luuid \


message ("DIR_IRODS_HOME _$$DIR_IRODS_HOME"_)
!isEqual (DIR_IRODS_HOME, "") {
 LIBS += -l$$GRASSROOTS_IRODS_LIB_NAME \
		-L$$DIR_IRODS_LIBS \
		-lirods_common \
		-lirods_client \
		-lirods_plugin_dependencies
 }

#
message ("GRASSROOTS_DRMAA_LIB_NAME _$$GRASSROOTS_DRMAA_LIB_NAME"_)
!isEqual(GRASSROOTS_DRMAA_LIB_NAME,"") {
		message ("added GRASSROOTS_DRMAA_LIB_NAME _$$GRASSROOTS_DRMAA_LIB_NAME"_)
		LIBS += -l$$GRASSROOTS_DRMAA_LIB_NAME \
		-L$$DIR_DRMAA_LIB -ldrmaa \
		# $$DRMAA_DEPS
}

target.path = $$DIR_GRASSROOTS_INSTALL_ROOT
INSTALLS += target

OTHER_FILES += \
		dependencies.pri \
		example_dependencies.pri

DISTFILES += \
		dependencies.pri
