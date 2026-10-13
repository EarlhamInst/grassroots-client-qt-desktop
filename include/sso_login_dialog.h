/*
** Copyright 2014-2026 The Earlham Institute
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

/**
 * @file
 * @brief A dialog that lets the user sign in to a Grassroots server
 * protected by web-based single sign-on such as the Apache module
 * mod_auth_openidc.
 */
#ifndef SSO_LOGIN_DIALOG_H
#define SSO_LOGIN_DIALOG_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QNetworkCookie>
#include <QUrl>


class QLabel;
class QWebEngineProfile;
class QWebEngineView;


/**
 * @brief Show the server's single sign-on pages in an embedded browser and
 * collect the session cookies that the server sets once the user has
 * signed in.
 *
 * The browser uses an off-the-record profile so nothing is stored on disk.
 * The dialog is accepted once the browser has returned to the server with
 * a mod_auth_openidc session cookie.
 */
class SSOLoginDialog : public QDialog
{
public:
	SSOLoginDialog (const QUrl &server_url_r, QWidget *parent_p = nullptr);
	virtual ~SSOLoginDialog ();

	/**
	 * Get all of the cookies that the server has set during the sign in,
	 * mod_auth_openidc can split large sessions over several cookies so
	 * they are all needed.
	 */
	QList <QNetworkCookie> GetServerCookies () const;

private:
	QUrl sld_server_url;
	QWebEngineProfile *sld_profile_p;
	QWebEngineView *sld_view_p;
	QLabel *sld_location_p;

	/** The cookies for the server, keyed by name and path */
	QMap <QString, QNetworkCookie> sld_cookies;

	bool IsServerCookie (const QNetworkCookie &cookie_r) const;
	bool HasSessionCookie () const;
	static QString GetCookieKey (const QNetworkCookie &cookie_r);
};


#endif		/* #ifndef SSO_LOGIN_DIALOG_H */
