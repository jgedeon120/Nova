var spawn = require('child_process').spawn;

var updater = {
	updateInProgress: false
	, procOutput: String()
};

updater.IsUpdateInProgress = function() {
    return this.updateInProgress;
}

updater.AutoUpdate = function(cb) {
    var self = this;
	if (!self.updateInProgress) {
		self.updateInProgress = true;
    
		var executionString = "sudo";
		var args = new Array();

		args.push("/bin/bash");
		args.push("/usr/share/nova/sharedFiles/autoUpdate.sh");
	 
		self.proc = spawn(executionString.toString(), args);
    
		self.proc.stdout.on('data', function (data){
			self.procOutput += String(data);
		});
    
		self.proc.stderr.on('data', function (data){
	  		self.procOutput += String(data);
		});
    
		self.proc.on('exit', function (code){
	  		self.updateInProgress = false;
	  		self.procOutput = "";
      		
			if (code == 0) {
        		LOG("ALERT", "Quasar is exiting due to Nova update");
        		process.exit(1);
      		}
		});
	} else {
		cb(null, self.procOutput);
	}
    
	self.proc.stdout.on('data', function (data){
        cb(null, String(data));
    });
            
    self.proc.stderr.on('data', function (data){
      cb(null, String(data));
    });
		
	self.proc.on('exit', function (code){
    	cb(null, 'UPDATER exited with code ' + code);
	});
    
}

module.exports = updater;
