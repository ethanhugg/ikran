/* Expose API under window.navigator.service.media */
if (window && window.navigator) {
    if (!window.navigator.service)
        window.navigator.service = {};
    window.navigator.service.call= {
        registerUser: function(user_device, user, credential,proxy_address, sess_obs) {
            return regUser(window.location, user_device, user, credential, proxy_address, sess_obs);
        },
        placeCall: function(dn, ctx, media_obs) {
            return callStart(window.location, dn, ctx, media_obs);
        },
        startP2PMode: function(user, sess_obs) {
            return startP2PMode(window.location, user, sess_obs);
        },
        placeP2PCall: function(dn, ip_address, ctx, media_obs) {
            return callP2PStart(window.location, dn, ip_address, ctx, media_obs);
        },        
        hangupCall: function() {
            return callStop(window.location);
        },
        unregisterUser: function() {
            return unregUser(window.location);
        },
        answerCall: function(ctx, media_obs) {
            return callAnswer(window.location, ctx, media_obs);
        },
        setProperty: function(name, value) {
            return callSetProperty(window.location, name, value);
        },
        getProperty: function(name) {
        	var value = callGetProperty(window.location, name);
            return value;
        },  
        sendDigits: function(digits) {
        	return callSendDigits(window.location, digits);
        },            
  		fetchImage: function(isFile) {
            return fetchImg(window.location, isFile);
        }
    }
}

