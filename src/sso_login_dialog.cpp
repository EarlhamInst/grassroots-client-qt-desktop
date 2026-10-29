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

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QWebEngineCookieStore>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>

#include "sso_login_dialog.h"


/** The prefix of the names of the session cookies set by mod_auth_openidc */
static const char * const S_SESSION_COOKIE_PREFIX_S = "mod_auth_openidc_session";


SSOLoginDialog :: SSOLoginDialog (const QUrl &server_url_r, QWidget *parent_p)
: QDialog (parent_p), sld_server_url (server_url_r)
{
	QVBoxLayout *layout_p = new QVBoxLayout (this);
	QLabel *message_p = new QLabel (tr ("Please sign in to %1 to continue.").arg (server_url_r.host ()), this);
	QDialogButtonBox *buttons_p = new QDialogButtonBox (QDialogButtonBox :: Cancel, this);

	setWindowTitle (tr ("Sign in to %1").arg (server_url_r.host ()));

	/* A profile without a storage name is off-the-record so nothing is saved to disk */
	sld_profile_p = new QWebEngineProfile (this);

	sld_view_p = new QWebEngineView (this);
	sld_view_p -> setPage (new QWebEnginePage (sld_profile_p, sld_view_p));

	/* Show where we are so the user can check the sign in page is genuine */
	sld_location_p = new QLabel (this);
	sld_location_p -> setTextInteractionFlags (Qt :: TextSelectableByMouse);


	/*
	 * The SSO url can be very long so we need to make sure that
	 * the dialog doesn't become crazily wide due to the location
	 * label being massive, so set the size policy.
	 */
	sld_location_p -> setSizePolicy (QSizePolicy :: Ignored, QSizePolicy :: Preferred);
	sld_location_p -> setMinimumWidth (0);

	layout_p -> addWidget (message_p);
	layout_p -> addWidget (sld_location_p);
	layout_p -> addWidget (sld_view_p, 1);
	layout_p -> addWidget (buttons_p);

	connect (buttons_p, &QDialogButtonBox :: rejected, this, &QDialog :: reject);

	connect (sld_view_p, &QWebEngineView :: urlChanged, this, [this] (const QUrl &url_r)
		{
			const QString url_s = url_r.toDisplayString ();

			sld_location_p -> setText (url_s);
			sld_location_p -> setToolTip (url_s);
		});


	QWebEngineCookieStore *cookie_store_p = sld_profile_p -> cookieStore ();

	connect (cookie_store_p, &QWebEngineCookieStore :: cookieAdded, this, [this] (const QNetworkCookie &cookie_r)
		{
			if (IsServerCookie (cookie_r))
				{
					sld_cookies.insert (GetCookieKey (cookie_r), cookie_r);
				}
		});

	connect (cookie_store_p, &QWebEngineCookieStore :: cookieRemoved, this, [this] (const QNetworkCookie &cookie_r)
		{
			sld_cookies.remove (GetCookieKey (cookie_r));
		});


	/*
	 * Once the identity provider has sent the browser back to the server,
	 * the session cookie will have been set and we have finished.
	 */
	connect (sld_view_p, &QWebEngineView :: loadFinished, this, [this] (bool /* ok_flag */)
		{
			/*
			 * The server itself may return an error page for a browser request
			 * so we only care that we are back on it with a session.
			 */
			if ((sld_view_p -> url ().host () == sld_server_url.host ()) && (HasSessionCookie ()))
				{
					accept ();
				}
		});

	resize (900, 700);

	sld_view_p -> load (server_url_r);
}


SSOLoginDialog :: ~SSOLoginDialog ()
{
	/* The page must be deleted before the profile that it uses */
	delete sld_view_p;
	sld_view_p = nullptr;
}


QList <QNetworkCookie> SSOLoginDialog :: GetServerCookies () const
{
	return sld_cookies.values ();
}


bool SSOLoginDialog :: IsServerCookie (const QNetworkCookie &cookie_r) const
{
	const QString host = sld_server_url.host ();
	QString domain = cookie_r.domain ();

	if (domain.isEmpty ())
		{
			return true;
		}

	if (domain.startsWith ('.'))
		{
			domain.remove (0, 1);
		}

	return ((host.compare (domain, Qt :: CaseInsensitive) == 0) || (host.endsWith ("." + domain, Qt :: CaseInsensitive)));
}


bool SSOLoginDialog :: HasSessionCookie () const
{
	for (const QNetworkCookie &cookie : sld_cookies)
		{
			if (cookie.name ().startsWith (S_SESSION_COOKIE_PREFIX_S))
				{
					return true;
				}
		}

	return false;
}


QString SSOLoginDialog :: GetCookieKey (const QNetworkCookie &cookie_r)
{
	return QString :: fromUtf8 (cookie_r.name ()) + "|" + cookie_r.domain () + "|" + cookie_r.path ();
}
