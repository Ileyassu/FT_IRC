// Add this method to the end of multiplexing.cpp

void Multiplexer::cleanupEmptyChannels()
{
	std::vector<std::string> channelsToRemove;
	
	for (std::map<std::string, Channel*>::iterator it = channels.begin(); 
		 it != channels.end(); ++it) {
		if (it->second->getMemberCount() == 0) {
			channelsToRemove.push_back(it->first);
		}
	}
	
	for (std::vector<std::string>::iterator it = channelsToRemove.begin(); 
		 it != channelsToRemove.end(); ++it) {
		removeChannel(*it);
	}
}
