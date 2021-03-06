/*
 * Copyright (C) 2010 Emweb bvba, Leuven, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#include "Wt/WServer"
#include "IsapiStream.h"
#include "Server.h"
#include "Wt/WLogger"
#include "WebMain.h"
#include "WebController.h"

#include <boost/algorithm/string.hpp>

#include <Windows.h>
#include <fstream>

namespace Wt {

static WebMain *webMain = 0;

struct WServer::Impl {
  Impl()
    : running_(false)
  { }

  bool running_;
};

WServer::WServer(const std::string& applicationPath,
		 const std::string& configurationFile)
  : impl_(new Impl())
{
  init(applicationPath, configurationFile);
  if (!isapi::IsapiServer::instance()->addServer(this))
    throw Exception("WServer::WServer(): "
		    "Only one simultaneous WServer supported");
  instance_ = this;

  std::stringstream approotLog;
  std::string approot;
  {
    std::string inifile = applicationPath + ".ini";
    char buffer[1024];
    GetPrivateProfileString("isapi", "approot", "",
      buffer, sizeof(buffer), inifile.c_str());
    approot = buffer;
    if (approot != "") {
      approotLog << "ISAPI: read approot (" << approot
        << ") from ini file " << inifile;
    }
  }

  setAppRoot(approot);
  setConfiguration(configurationFile);
  if (approotLog.str() != "") {
    log("notice") << approotLog.str();
  }
}

WServer::~WServer()
{
  isapi::IsapiServer::instance()->removeServer(this);
  delete impl_;
}

void WServer::setServerConfiguration(int argc, char *argv[],
				     const std::string& serverConfigurationFile)
{

}

//void WServer::addEntryPoint(EntryPointType type, ApplicationCreator callback,
//			    const std::string& path, const std::string& favicon)
//{
//  if (!impl_->configuration())
//    throw Exception("WServer::addEntryPoint(): "
//		    "call setServerConfiguration() first");
//
//  impl_->configuration()
//    ->addEntryPoint(EntryPoint(type, callback, path, favicon));
//}

bool WServer::start()
{
  if (isRunning()) {
    log("error") << "WServer::start() error: server already started!";
    return false;
  }

  impl_->running_ = true;
  webController_ = new Wt::WebController(*this, "", false);

  try {
    isapi::IsapiStream isapiStream(isapi::IsapiServer::instance());

    WebMain requestServer(this, &isapiStream);
    webMain = &requestServer;

    requestServer.run();

    webMain = 0;

  } catch (std::exception& e) {
    log("fatal") << "ISAPI server: caught unhandled exception: " << e.what();

    throw;
  } catch (...) {
    log("fatal") << "ISAPI server: caught unknown, unhandled exception.";
    throw;
  }
  return true;
}

bool WServer::isRunning() const
{
  return impl_ && impl_->running_;
}

int WServer::httpPort() const
{
  return -1;
}

void WServer::stop()
{
  if (!isRunning()) {
    std::cerr << "WServer::stop() error: server not yet started!" << std::endl;
    return;
  }
  webMain->shutdown();
}

//int WServer::waitForShutdown(const char *restartWatchFile)
//{
//  for (;;)
//    Sleep(10000 * 1000);
//
//  return 0;
//}

//void WServer::handleRequest(WebRequest *request)
//{
//  webMain->controller().handleRequest(request);
//}

//void WServer::schedule(int milliSeconds,
//		       const boost::function<void ()>& function)
//{
//  webMain->schedule(milliSeconds, function);
//}

//void WServer::post(const std::string& sessionId,
//		   const boost::function<void ()>& function,
//		   const boost::function<void ()>& fallbackFunction)
//{
//  schedule(0, sessionId, function, fallbackFunction);
//}

//void WServer::schedule(int milliSeconds,
//		       const std::string& sessionId,
//		       const boost::function<void ()>& function,
//		       const boost::function<void ()>& fallbackFunction)
//{
//  ApplicationEvent event(sessionId, function, fallbackFunction);
//
//  schedule(milliSeconds, boost::bind(&WebController::handleApplicationEvent,
//				     &webMain->controller(), event));
//}

//std::string WServer::appRoot() const
//{
//  return impl_->configuration()->appRoot();
//}

//void WServer::initializeThread()
//{ }

//bool WServer::usesSlashExceptionForInternalPaths() const
//{
//  // Is not relevent, one cannot even deploy at a path ending with '/' ?
//  return false;
//}

//bool WServer::readConfigurationProperty(const std::string& name,
//                                        std::string& value) const
//{
//  return impl_->configuration()->readConfigurationProperty(name, value);
//}

int WRun(int argc, char *argv[], ApplicationCreator createApplication)
{
  try {
    WServer server(argv[0], "");

    try {
      server.setServerConfiguration(argc, argv);
      server.addEntryPoint(Application, createApplication);
      server.start();

      return 0;
    } catch (std::exception& e) {
      server.log("fatal") << e.what();
      return 1;
    }
  } catch (Wt::WServer::Exception&) {
    //std::cerr << e.what() << std::endl;
    return 1;
  } catch (std::exception&) {
    //std::cerr << "exception: " << e.what() << std::endl;
    return 1;
  }
}

}
