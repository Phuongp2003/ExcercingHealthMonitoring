/**
 * Data Handler - Manages sensor data, history, and processing
 */

class DataHandler {
	constructor() {
		// Data history array - stores the last 2 minutes of data
		this.dataHistory = [];
		this.lastDataInfo = null;
		this.DATA_RETENTION_TIME = 120000; // 2 minutes in milliseconds

		// Set up periodic cleanup
		setInterval(() => this.cleanupDataHistory(), 30000); // Check every 30 seconds
	}

	/**
	 * Add data to history and trigger UI update
	 */
	addToDataHistory(data) {
		if (!data) return;

		// Store the last data information
		this.lastDataInfo = data;

		// Create a history item with current timestamp
		const historyItem = {
			...data,
			historyTimestamp: Date.now(),
		};

		// Add to beginning of array
		this.dataHistory.unshift(historyItem);

		// Clean up old data (older than 2 minutes)
		this.cleanupDataHistory();

		// Return the updated history
		return this.dataHistory;
	}

	/**
	 * Remove data older than 2 minutes
	 */
	cleanupDataHistory() {
		const now = Date.now();
		this.dataHistory = this.dataHistory.filter(
			(item) => now - item.historyTimestamp <= this.DATA_RETENTION_TIME
		);
		return this.dataHistory;
	}

	/**
	 * Get the activity info (name and class) from activity class
	 */
	getActivityInfo(activityClass) {
		switch (activityClass) {
			case 0:
				return { name: 'Resting after exercise', className: 'resting' };
			case 1:
				return { name: 'Sitting', className: 'sitting' };
			case 2:
				return { name: 'Walking', className: 'walking' };
			default:
				return { name: 'Unknown', className: '' };
		}
	}

	/**
	 * Get the last data information
	 */
	getLastData() {
		return this.lastDataInfo;
	}

	/**
	 * Get the data history
	 */
	getDataHistory() {
		return this.dataHistory;
	}
}

// Create global instance
const dataHandler = new DataHandler();
