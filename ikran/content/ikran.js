/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Rainbow.
 *
 * The Initial Developer of the Original Code is Mozilla Labs.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Anant Narayanan <anant@kix.in>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const nsISupports = Components.interfaces.nsISupports;
const nsIFactory = Components.interfaces.nsIFactory;
const nsIInterfaceRequestor = Components.interfaces.nsIInterfaceRequestor;
const nsIDomWindow = Components.interfaces.nsIDomWindow;
const nsIWebNavigation = Components.interfaces.nsIWebNavigation;
const nsIDocShellTreeItem = Components.interfaces.nsIDocShellTreeItem;
const nsIBaseWindow = Components.interfaces.nsIBaseWindow;
const nsIXULWindow = Components.interfaces.nsIXULWindow;
var parentWin = null;
var parent;


const PREFNAME = "allowedDomains";
const DEFAULT_DOMAINS = [
    "http://localhost",
    "http://mozilla.github.com"
];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function Ikran() {
    this._session = false;
    this._call = false;
}
Ikran.prototype = {
    get _prefs() {
        delete this._prefs;
        return this._prefs =
            Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService).getBranch("extensions.ikran.").
                    QueryInterface(Ci.nsIPrefBranch2);
    },
    
    get _perms() {
        delete this._perms;
        return this._perms = 
            Cc["@mozilla.org/permissionmanager;1"].
                getService(Ci.nsIPermissionManager);
    },
    
    get _ikran() {
        delete this._callcontrol;
        return this._callcontrol =
            Cc["@cto.cisco.com/call/control;1"].
                getService(Ci.ICallControl)
    },
    
    _makeURI: function(url) {
        let iosvc = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
        return iosvc.newURI(url, null, null);
    },
    
    _makePropertyBag: function(prop) {
        let iP = ["localvoipport", "remotevoipport", "version"];
        let bP = ["udp", "tcp"];

        let bag = Cc["@mozilla.org/hash-property-bag;1"].
            createInstance(Ci.nsIWritablePropertyBag2);

        iP.forEach(function(p) {
            if (p in prop)
                bag.setPropertyAsAString(p, prop[p]);
        });
        bP.forEach(function(p) {
            if (p in prop)
                bag.setPropertyAsBool(p, prop[p]);
        });
        
        return bag;
    },
    
    _verifyPermission: function(win, loc, cb) {
        let location = loc.protocol + "//" + loc.hostname;
        if (loc.port) location += ":" + loc.port;
        
        // Domains in the preference are always allowed
        let found = false;
        if (this._prefs.getPrefType(PREFNAME) ==
            Ci.nsIPrefBranch.PREF_INVALID) {
            this._prefs.setCharPref(PREFNAME, JSON.stringify(DEFAULT_DOMAINS));
        }
        let domains = JSON.parse(this._prefs.getCharPref(PREFNAME));
        domains.forEach(function(domain) {
            if (location == domain) { cb(true); found = true; }
        });
        if (found)
            return;
            
        // If domain not found in preference, check permission manager
        let self = this;
        let uri = this._makeURI(location);
        switch (this._perms.testExactPermission(uri, "ikran")) {
            case Ci.nsIPermissionManager.ALLOW_ACTION:
                cb(true); break;
            case Ci.nsIPermissionManager.DENY_ACTION:
                cb(false); break;
            case Ci.nsIPermissionManager.UNKNOWN_ACTION:
                // Ask user
                let acceptButton = {
                    label: "Yes", accessKey: "y",
                    callback: function() {
                        self._perms.add(uri, "ikran", Ci.nsIPermissionManager.ALLOW_ACTION);
                        cb(true);
                    }
                };
                let declineButton = {
                    label: "No", accessKey: "n",
                    callback: function() {
                        self._perms.add(uri, "ikran", Ci.nsIPermissionManager.DENY_ACTION);
                        cb(false);
                    }
                };
                
                let message =
                    "This website is requesting access to your webcam " +
                    "and microphone. Do you wish to allow it?";
                
                let ret = win.PopupNotifications.show(
                    win.gBrowser.selectedBrowser,
                    "ikran-access-request",
                    message, null, acceptButton, [declineButton], {
                        "persistence": 1,
                        "persistWhileVisible": true,
                        "eventCallback": function(state) {
                            // Dismissing prompt implies immediate denial
                            // but we do not persist the choice
                            if (state == "dismissed") {
                                cb(false); ret.remove();
                            }
                        }
                    }
                );
        }
    },
    
    registerUser: function(user_device, user,credential, proxy,  obs) {
        if (this._session)
            throw "Session already in progress";

        // Make sure observer is setup correctly, if none provided, ignore
        if (obs) this._session_observer = obs;
        else this._session_observer = function() {};
        
        // Start rainbow session
        this._ikran.registerUser(user_device, user, credential, proxy, this._session_observer);
        this._session = true;
    },

    startP2PMode: function(user, obs) {
        if (this._session)
            throw "Session already in progress";

        // Make sure observer is setup correctly, if none provided, ignore
        if (obs) this._session_observer = obs;
        else this._session_observer = function() {};
        
        // Start rainbow session
        this._ikran.startP2PMode( user, this._session_observer);
        this._session = true;
    },
    
    placeCall: function(dn, ctx, obs) {
            
        // Make sure observer is setup correctly, if none provided, ignore
        if (obs) this._media_observer = obs;
        else this._media_observer = function() {};

        this._ikran.placeCall(dn, ctx, this._media_observer);
    },

    placeP2PCall: function(dn, ip_address, ctx, obs) {
            
        // Make sure observer is setup correctly, if none provided, ignore
        if (obs) this._media_observer = obs;
        else this._media_observer = function() {};

        this._ikran.placeP2PCall(dn, ip_address, ctx, this._media_observer);
    },
    
    hangupCall: function() {

        this._ikran.hangupCall();

    },
    
    unregisterUser: function() {
        this._ikran.unregisterUser();
    },

    answerCall: function(ctx, obs) {

        // Make sure observer is setup correctly, if none provided, ignore
        if (obs) this._media_observer = obs;
        else this._media_observer = function() {};
        this._ikran.answerCall(ctx,obs);
	},
	
    setProperty: function(prop) {
    	// Make property bag
        let bag = this._makePropertyBag(prop);
    	
        this._ikran.setProperty(bag);
    },	

    getProperty: function(name) {
    	var value = {};
        this._ikran.getProperty(name, value);
        return value;
    },
    
    sendDigits: function(digits) {
    	this._ikran.sendDigits(digits);
    }

	
};

var EXPORTED_SYMBOLS = ["Ikran"];
