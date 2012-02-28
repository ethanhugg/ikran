/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO)
#include "HTMLSessionElement.h"

#include "Attribute.h"
#include "ContentType.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FrameView.h"
#include "HTMLDocument.h"
#include "HTMLNames.h"
#include "MediaPlayer.h"
#include "HTMLVideoElement.h"
#include "Logging.h"
#include "Page.h"
#include "ScriptEventListener.h"
#include <limits>
#include <wtf/text/CString.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames; 

#ifndef LOG_MEDIA_EVENTS
// Default to not logging events because so many are generated they can overwhelm the rest of 
// the logging.
#define LOG_MEDIA_EVENTS 0
#endif

/*
HTMLSessionElement::HTMLSessionElement(const QualifiedName& tagName, Document* document, HTMLVideoElement *videoPane, String& aor, String& credentials, String& proxy)
    : HTMLElement(tagName, document)
    , m_player(0)
    , m_volume(1.0f)
    , m_sessionState(NO_SESSION)
    , m_regState(NO_REGISTRAR)
    , m_asyncEventTimer(this, &HTMLSessionElement::asyncEventTimerFired)
{
    LOG(Media, "HTMSession::HTMLSession");
  ASSERT(hasTagName(sessionTag));
}
*/

HTMLSessionElement::HTMLSessionElement(const QualifiedName& tagName, Document* document) 
    : HTMLElement(tagName, document)
    ,m_volume(1.0f)
    , m_sessionState(NO_SESSION)
    , m_regState(NO_REGISTRAR)
    , m_asyncEventTimer(this, &HTMLSessionElement::asyncEventTimerFired)
	, m_errorEventTimer(this, &HTMLSessionElement::errorEventTimerFired)

{
    LOG(Media, "HTMSession::HTMLSession");
  ASSERT(hasTagName(sessionTag));
}

HTMLSessionElement::~HTMLSessionElement()
{
    LOG(Media, "HTMLSession::~HTMLSession");
	
}

//void HTMLSessionElement::setMediaPlayer(MediaPlayer* mplayer)
//{
//	m_player = mplayer;
//}

void HTMLSessionElement::setMediaControlElement(HTMLMediaElement* mElement)
{
   m_element = mElement;
}

PassRefPtr<HTMLSessionElement> HTMLSessionElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLSessionElement(tagName, document));
}

void HTMLSessionElement::insertedIntoTree(bool deep)
{
    HTMLElement::insertedIntoTree(deep);
    if (parentNode() && (parentNode()->hasTagName(audioTag) || parentNode()->hasTagName(videoTag)))
       static_cast<HTMLMediaElement*>(parentNode())->sessionWasAdded(this);
}

void HTMLSessionElement::willRemove()
{
    if (parentNode() && (parentNode()->hasTagName(audioTag) || parentNode()->hasTagName(videoTag)))
        static_cast<HTMLMediaElement*>(parentNode())->sessionWillBeRemoved(this);
    HTMLElement::willRemove();
}


void HTMLSessionElement::open()
{
	LOG(Media, "Session: Open ");
}

void HTMLSessionElement::accept(bool accept)
{
	LOG(Media, "Session: Accept");
	m_element->openSessionInternal();
}

void HTMLSessionElement::close()
{
	LOG(Media, "Session: Close");

}

bool HTMLSessionElement::isURLAttribute(Attribute* attribute) const
{
    return attribute->name() == srcAttr;
}

//bool HTMLSessionElement::sendKeyPress(String& key)
//{
 //return true;
//}


void HTMLSessionElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    HTMLElement::attributeChanged(attr, preserveDecls);

    const QualifiedName& attrName = attr->name();
    if (attrName == srcAttr) {
        // Trigger a reload, as long as the 'src' attribute is present.
        if (!getAttribute(srcAttr).isEmpty())
	{
	}	
    }
    else if (attrName == aorAttr) {
    }
    else if (attrName == credentialsAttr) {

    }
    else if (attrName == proxyAttr) {

    }
}



void HTMLSessionElement::parseMappedAttribute(Attribute* attr)
{
    const QualifiedName& attrName = attr->name();

    if (attrName == onregistrationstateAttr)
        setAttributeEventListener(eventNames().regstateEvent, createAttributeEventListener(this, attr));
    else if (attrName == onsessionstateAttr)
        setAttributeEventListener(eventNames().sessionstateEvent, createAttributeEventListener(this, attr));
    else
        HTMLElement::parseMappedAttribute(attr);
}



void HTMLSessionElement::scheduleEvent(const AtomicString& eventName)
{
    LOG(Media, "HTMLSessionElement::scheduleEvent - scheduling '%s'", eventName.string().ascii().data());
    m_pendingEvents.append(Event::create(eventName, false, true));
    //if (!m_asyncEventTimer.isActive())
        m_asyncEventTimer.startOneShot(0);
}

void HTMLSessionElement::asyncEventTimerFired(Timer<HTMLSessionElement>*)
{
    Vector<RefPtr<Event> > pendingEvents;
    ExceptionCode ec = 0;

    m_pendingEvents.swap(pendingEvents);
    unsigned count = pendingEvents.size();
    for (unsigned ndx = 0; ndx < count; ++ndx) {
        LOG(Media, "HTMLSessionElement::asyncEventTimerFired - dispatching '%s'", pendingEvents[ndx]->type().string().ascii().data());
        if (pendingEvents[ndx]->type() == eventNames().canplayEvent) {
            dispatchEvent(pendingEvents[ndx].release(), ec);
        } else
            dispatchEvent(pendingEvents[ndx].release(), ec);
    }
}

void HTMLSessionElement::defaultEventHandler(Event* event)
{
    HTMLElement::defaultEventHandler(event);
}


void HTMLSessionElement::setSrc(const String& url)
{
    setAttribute(srcAttr, url);
	m_currentSrc = url;	
}

String HTMLSessionElement::aor() const
{
	return getAttribute(aorAttr);
}

void HTMLSessionElement::setAor(const String& aor) 
{
	setAttribute(aorAttr,aor);
	m_aor = aor;
}

String HTMLSessionElement::credentials() const
{
	return getAttribute(credentialsAttr);
}

void HTMLSessionElement::setCredentials(const String& creds) 
{
	setAttribute(credentialsAttr,creds);
	m_credentials = creds;
}

String HTMLSessionElement::proxy() const
{
	return getAttribute(proxyAttr);
}

void HTMLSessionElement::setProxy(const String& proxy) 
{
	setAttribute(proxyAttr,proxy);
	m_proxy = proxy;
}

String HTMLSessionElement::dialnumber() const 
{
	return getAttribute(dialnumberAttr);
}

void HTMLSessionElement::setDialNumber(const String& number)
{
	setAttribute(dialnumberAttr, number);
	m_dialNumber = number;
}

void HTMLSessionElement::scheduleErrorEvent()
{
    LOG(Media, "HTMLSessionElement::scheduleErrorEvent - %p", this);
    if (m_errorEventTimer.isActive())
        return;

    m_errorEventTimer.startOneShot(0);
}

void HTMLSessionElement::cancelPendingErrorEvent()
{
    LOG(Media, "HTMLSessionElement::cancelPendingErrorEvent - %p", this);
    m_errorEventTimer.stop();
}

void HTMLSessionElement::errorEventTimerFired(Timer<HTMLSessionElement>*)
{
    LOG(Media, "HTMLSessionElement::errorEventTimerFired - %p", this);
    dispatchEvent(Event::create(eventNames().errorEvent, false, true));
}


String HTMLSessionElement::currentSrc() const
{
    return m_currentSrc;
}

float HTMLSessionElement::volume() const
{

    return m_volume;
}

void HTMLSessionElement::setVolume(float vol)
{
    LOG(Media, "HTMLSessionElement::setVolume(%f)", vol);

    if (vol < 0.0f || vol > 1.0f) {
        //ec = INDEX_SIZE_ERR;
        return;
    }
    
    if (m_volume != vol) {
        m_volume = vol;
        scheduleEvent(eventNames().volumechangeEvent);
    }
}

bool HTMLSessionElement::muted() const
{
    return m_muted;
}

void HTMLSessionElement::setMuted(bool muted)
{
    //LOG(Media, "HTMLSessionElement::setMuted(%s)", boolString(muted));

    if (m_muted != muted) {
        m_muted = muted;
        scheduleEvent(eventNames().volumechangeEvent);
    }
}


bool HTMLSessionElement::secure() const
{

  return m_secure;
}

void HTMLSessionElement::setSendVideo(bool value)
 {

     m_sendVideo = value;
}

bool HTMLSessionElement::sendVideo() const
 {

	return m_sendVideo;
 }

void HTMLSessionElement::setRegState(MediaPlayer::SipRegistrationState state)
{
        m_regState = static_cast<RegistrationState>(state);
    	scheduleEvent(eventNames().regstateEvent);
}

void HTMLSessionElement::setSessionState(MediaPlayer::SipSessionState state)
{
     m_sessionState = static_cast<SessionState>(state);
	//if(m_sessionState == ACCEPTING_SESSION)
	//{
	//	caller_name = m_player->callingPartyName();
	//	caller_number = m_player->callingPartyNumber();
//	}
    	scheduleEvent(eventNames().sessionstateEvent);
}

//events
String HTMLSessionElement::sipRegState() const
{
	String state="invalid";
	switch(m_regState) {
		case NO_REGISTRAR:
			state = "not_registered";
			break;
		case REGISTERING:
			state = "registering";
			break;
		case REGISTERED:
			state = "registered";
			break;
		case REGISTRATION_FAILED: 
			state = "registration_failed";
			break;
	}	
    LOG(Media," HTMLSessionElement::sipRegState : %s", state.ascii().data());
	return state;

}

String HTMLSessionElement::sipSessionState() const
{
    String state="invalid";
    switch(m_sessionState) {
        case NO_SESSION:
            state = "no_session";
            break;
        case OPENING_SESSION:
            state = "opening_session";
            break;
        case ACCEPTING_SESSION: 
            state = "accepting_session";
            break;
        case IN_SESSION:
            state = "in_session";
            break;
        case CLOSING_SESSION:
            state = "closing_session";
            break;
    }  
    LOG(Media," HTMLSessionElement::sipSessionState : %s", state.ascii().data());
    return state;

}

//TBD: make parameters reference based
void HTMLSessionElement::setIncomingCallInfo(String callerName, String callerNumber)
{

	 caller_name = callerName;
     caller_number = callerNumber;
}


void HTMLSessionElement::setSessionId(int id)
{
	session_id = id;
}

String HTMLSessionElement::callerName() const 
{
	return caller_name;
}

String HTMLSessionElement::callerNumber() const 
{
	return caller_number;
}

}
#endif
