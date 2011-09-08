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
        hangupCall: function() {
            return callStop(window.location);
        },
        unregisterUser: function() {
            return unregUser(window.location);
        },
        answerCall: function(ctx, media_obs) {
            return callAnswer(window.location, ctx, media_obs);
        },
  	fetchImage: function(isFile) {
            return fetchImg(window.location, isFile);
        }
    }
}

