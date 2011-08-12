/* Expose API under window.navigator.service.media */
if (window && window.navigator) {
    if (!window.navigator.service)
        window.navigator.service = {};
    window.navigator.service.call= {
        registerUser: function(user_name, user_password, proxy_address, sess_obs) {
            return regUser(window.location, user_name, user_password, proxy_address, sess_obs);
        },
        placeCall: function(dn, media_obs) {
            return callStart(window.location, dn, media_obs);
        },
        hangupCall: function() {
            return callStop(window.location);
        },
        unregisterUser: function() {
            return unregUser(window.location);
        },
        answerCall: function(media_obs) {
            return callAnswer(window.location, media_obs);
        },
  	fetchImage: function(isFile) {
            return fetchImg(window.location, isFile);
        }
    }
}

