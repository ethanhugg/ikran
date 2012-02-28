/*
 * Copyright (C) 2007, 2008, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLSessionElement_h
#define HTMLSessionElement_h

#if ENABLE(VIDEO)

#include "HTMLElement.h"
#include "MediaPlayer.h"
#include "HTMLVideoElement.h"
#include "HTMLMediaElement.h"



namespace WebCore {
class Event;

class HTMLSessionElement : public HTMLElement {
public:
    static PassRefPtr<HTMLSessionElement> create(const QualifiedName&, Document* );
   
	//session atrributes
	String aor() const;
	String credentials() const;
	String proxy() const;
	String dialnumber() const;

	void setAor(const String&);
	void setCredentials(const String&);
	void setProxy(const String&);
	void setSrc(const String&);
	void setDialNumber(const String&);
	  
	//error event handlers
	void scheduleErrorEvent();
    void cancelPendingErrorEvent();


   // session APIs
    void open();
    void close();
    void accept(bool accept);
    //bool sendKeyPress(String& key);

	//functions invoked by media player
	void setRegState(MediaPlayer::SipRegistrationState);
    void setSessionState(MediaPlayer::SipSessionState);
	void setIncomingCallInfo(String callerName, String callerNumber);
	void setSessionId(int id);
 
   //volume, muted , sendVideo  
    float volume() const;
    void setVolume(float);
    bool muted() const;
    void setMuted(bool);
    void setSendVideo(bool);
    bool sendVideo() const;

    bool secure() const;
 
   
   // registration states
   enum RegistrationState {NO_REGISTRAR, REGISTERING, REGISTERED, REGISTRATION_FAILED };
   String sipRegState() const;
   
   // session state
   enum SessionState { NO_SESSION, OPENING_SESSION, ACCEPTING_SESSION, IN_SESSION, CLOSING_SESSION };
   String sipSessionState() const;
    
   //src attribute
   String currentSrc() const;

	//void set//MediaPlayer(MediaPlayer* mplayer);	
	void setMediaControlElement(HTMLMediaElement* mElement);	
    String callerName() const;
	String callerNumber() const;
private:
    //constructors
    HTMLSessionElement(const QualifiedName&, Document*);
    ~HTMLSessionElement();
    //MediaPlayer* m_player;
    HTMLMediaElement* m_element;

	virtual void insertedIntoTree(bool);
    virtual void willRemove();
    virtual bool isURLAttribute(Attribute*) const;


    virtual void attributeChanged(Attribute*, bool preserveDecls);
    virtual void parseMappedAttribute(Attribute*);

    //contols on the session
    float m_volume;
    bool m_muted;
    bool m_sendVideo; 
   
   //session data
   String m_aor;
   String m_credentials;
   String m_proxy;
   bool m_secure;
   String m_currentSrc;
   String m_dialNumber;
   String caller_name;
   String caller_number;
   int    session_id;
   //session state
   SessionState m_sessionState;

   //registration state
   RegistrationState m_regState;


    //event handler
    void defaultEventHandler(Event*);
    void scheduleEvent(const AtomicString& eventName);
    void asyncEventTimerFired(Timer<HTMLSessionElement>*);
	void errorEventTimerFired(Timer<HTMLSessionElement>*);

    Timer<HTMLSessionElement> m_asyncEventTimer;
 	Timer<HTMLSessionElement> m_errorEventTimer;
    Vector<RefPtr<Event> > m_pendingEvents;
    RefPtr<HTMLVideoElement> m_videoPane;	

};

} //namespace

#endif
#endif
