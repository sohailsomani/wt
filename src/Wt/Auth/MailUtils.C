/*
 * Copyright (C) 2011 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include "Wt/WLogger"
#include "Wt/Mail/Client"

namespace Wt {
  namespace Auth {
    namespace MailUtils {
      void sendMail(const Mail::Message &m) {
	Mail::Client client;
	client.connect();
	client.send(m);
      }
    }
  }
}
