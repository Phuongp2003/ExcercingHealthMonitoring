/**
 * UI Controller - Manages all user interface interactions and updates
 */

class UIController {
	constructor() {
		// DOM elements
		this.elements = {
			connectionStatus: document.getElementById('connection-status'),
			startButton: document.getElementById('start-button'),
			stopButton: document.getElementById('stop-button'),
			clearConnectionButton: document.getElementById('clear-connection'),
			checkStatusButton: document.getElementById('check-status'),
			deviceStatusDiv: document.getElementById('device-status'),
			lastDataDiv: document.getElementById('last-data'),
			lastDataContainer: document.getElementById('last-data-container'),
			lastDataTime: document.getElementById('last-data-time'),
			collectionStatus: document.getElementById('collection-status'),
			statusText: document.getElementById('status-text'),
			toastContainer: document.getElementById('toast-container'),
			heartRateValue: document.getElementById('heart-rate-value'),
			oxygenValue: document.getElementById('oxygen-value'),
			activityBadge: document.getElementById('activity-badge'),
			confidenceValue: document.getElementById('confidence-value'),
			vitalsUpdateTime: document.getElementById('vitals-update-time'),
			dataHistoryContainer: document.getElementById('data-history'),
		};

		this.setupEventListeners();
	}

	/**
	 * Set up event listeners for buttons
	 */
	setupEventListeners() {
		// We'll implement these in app.js
	}

	/**
	 * Show a toast notification
	 */
	showToast(message, type = 'info', duration = 5000) {
		// Limit to maximum 7 toasts
		if (document.querySelectorAll('.toast').length >= 7) {
			// Remove the oldest toast
			const oldestToast = document.querySelector('.toast');
			if (oldestToast) {
				oldestToast.remove();
			}
		}

		// Create toast element
		const toast = document.createElement('div');
		toast.className = `toast ${type}`;
		toast.innerHTML = `
            <span class="toast-close" onclick="this.parentElement.remove()">×</span>
            ${message}
        `;

		// Add toast to container
		this.elements.toastContainer.appendChild(toast);

		// Trigger reflow to enable transition
		void toast.offsetWidth;

		// Add show class to start animation
		toast.classList.add('show');

		// Auto remove after duration
		setTimeout(() => {
			toast.classList.remove('show');
			setTimeout(() => {
				if (toast.parentElement) {
					toast.remove();
				}
			}, 500);
		}, duration);
	}

	/**
	 * Update the activity badge
	 */
	updateActivityBadge(activityClass) {
		const activityInfo = dataHandler.getActivityInfo(activityClass);
		this.elements.activityBadge.textContent = activityInfo.name;
		this.elements.activityBadge.className = `activity-badge ${activityInfo.className}`;
	}

	/**
	 * Show command status indicator
	 */
	showCommandStatus(command, status, message = '') {
		// Remove existing command status indicator if any
		const existingStatus = document.getElementById(
			'command-status-indicator'
		);
		if (existingStatus) {
			existingStatus.remove();
		}

		if (status === '') return; // Don't show if no status

		// Create new indicator
		const statusIndicator = document.createElement('div');
		statusIndicator.id = 'command-status-indicator';
		statusIndicator.className = `command-status ${status}`;

		switch (status) {
			case 'pending':
				statusIndicator.textContent = `${command} - Waiting for sensor...`;
				break;
			case 'success':
				statusIndicator.textContent = `${command} - Success`;
				break;
			case 'error':
				statusIndicator.textContent = `${command} - Failed`;
				break;
		}

		// Add status indicator near the corresponding button
		if (command === 'START') {
			this.elements.startButton.parentNode.insertBefore(
				statusIndicator,
				this.elements.startButton.nextSibling
			);
		} else if (command === 'STOP') {
			this.elements.stopButton.parentNode.insertBefore(
				statusIndicator,
				this.elements.stopButton.nextSibling
			);
		}

		// Auto-remove after 5 seconds for success/error
		if (status !== 'pending') {
			setTimeout(() => {
				if (statusIndicator.parentNode) {
					statusIndicator.remove();
				}
			}, 5000);
		}

		// Show toast message for errors
		if (status === 'error' && message) {
			this.showToast(`${command} failed: ${message}`, 'error');
		}
	}

	/**
	 * Add a sync indicator to the collection status div
	 */
	addSyncIndicator() {
		const existingIndicator = document.getElementById('sync-indicator');
		if (!existingIndicator) {
			const syncIndicator = document.createElement('div');
			syncIndicator.id = 'sync-indicator';
			syncIndicator.className = 'sync-indicator';
			syncIndicator.innerHTML = `
                <span class="dot fresh"></span>
                <span id="sync-text">Sensor data is up to date</span>
            `;
			this.elements.collectionStatus.appendChild(syncIndicator);
		}
	}

	/**
	 * Update the sync indicator based on time since last update
	 */
	updateSyncIndicator(timeSinceUpdate) {
		const syncIndicator = document.getElementById('sync-indicator');
		if (!syncIndicator) {
			this.addSyncIndicator();
			return;
		}

		const dot = syncIndicator.querySelector('.dot');
		const text = document.getElementById('sync-text');

		if (timeSinceUpdate < 0) {
			// No update received yet
			dot.className = 'dot old';
			text.textContent = 'Waiting for sensor data...';
		} else if (timeSinceUpdate < 10) {
			// Fresh data (less than 10 seconds old)
			dot.className = 'dot fresh';
			text.textContent = 'Sensor data is up to date';
		} else if (timeSinceUpdate < 30) {
			// Stale data (10-30 seconds old)
			dot.className = 'dot stale';
			text.textContent = `Sensor data is ${Math.round(
				timeSinceUpdate
			)}s old`;
		} else {
			// Old data (more than 30 seconds old)
			dot.className = 'dot old';
			text.textContent = `Sensor data is ${Math.round(
				timeSinceUpdate
			)}s old - check connection`;
		}
	}

	/**
	 * Update the main UI based on connection and collection state
	 */
	updateUI(state) {
		// Update connection status
		this.elements.connectionStatus.textContent = state.isConnected
			? 'Connected'
			: 'Disconnected';
		this.elements.connectionStatus.className =
			'status ' + (state.isConnected ? 'connected' : 'disconnected');

		// Update buttons based on device state and command in progress
		this.elements.checkStatusButton.disabled =
			!state.isConnected || state.commandInProgress;
		this.elements.clearConnectionButton.disabled = state.commandInProgress;

		// Add/update sync indicator
		if (state.isConnected) {
			this.addSyncIndicator();
		}

		// Update collection status indicator - treat both collecting and processing as active
		const isActivelyMeasuring =
			state.isCollecting || state.deviceCurrentState === 'PROCESSING';

		if (isActivelyMeasuring) {
			this.elements.collectionStatus.style.display = 'block';
			this.elements.collectionStatus.style.backgroundColor = '#dff0d8';
			this.elements.collectionStatus.style.color = '#3c763d';
			this.elements.statusText.textContent =
				state.deviceCurrentState === 'PROCESSING'
					? `Active - Processing Data (${state.deviceCurrentState})`
					: `Active - Collecting Data (${state.deviceCurrentState})`;
		} else if (state.isConnected) {
			this.elements.collectionStatus.style.display = 'block';
			this.elements.collectionStatus.style.backgroundColor = '#fcf8e3';
			this.elements.collectionStatus.style.color = '#8a6d3b';
			this.elements.statusText.textContent = `Ready - Not Collecting (${state.deviceCurrentState})`;
		} else {
			this.elements.collectionStatus.style.display = 'none';
		}

		// Also update button states accordingly - can only start when not collecting or processing
		this.elements.startButton.disabled =
			!state.isConnected ||
			isActivelyMeasuring ||
			state.commandInProgress;
		this.elements.stopButton.disabled =
			!state.isConnected ||
			!isActivelyMeasuring ||
			state.commandInProgress;
	}

	/**
	 * Update last data display with new data
	 */
	updateLastDataDisplay(data) {
		if (!data) return;

		// Highlight the container to show new data arrived
		this.elements.lastDataContainer.classList.add('highlight');
		setTimeout(() => {
			this.elements.lastDataContainer.classList.remove('highlight');
		}, 2000);

		// Update vital signs display
		if (data.heartRate) {
			this.elements.heartRateValue.textContent =
				data.heartRate.toFixed(1);
		}

		if (data.oxygenLevel) {
			this.elements.oxygenValue.textContent = data.oxygenLevel.toFixed(1);
		}

		if (data.actionClass !== undefined && data.actionClass >= 0) {
			this.updateActivityBadge(data.actionClass);
		}

		// Update confidence display
		if (data.confidence !== undefined) {
			this.elements.confidenceValue.textContent = `Confidence: ${(
				data.confidence * 100
			).toFixed(0)}%`;
		} else {
			this.elements.confidenceValue.textContent = 'Confidence: --';
		}

		// Update the vitals timestamp
		const now = new Date();
		this.elements.vitalsUpdateTime.textContent = `Updated: ${now.toLocaleTimeString()}`;

		// Format the display with more details for last data area
		let html = '';

		if (data.heartRate) {
			html += `<div class="data-info"><strong>Heart Rate:</strong> ${data.heartRate.toFixed(
				1
			)} BPM</div>`;
		}

		if (data.oxygenLevel) {
			html += `<div class="data-info"><strong>SpO2:</strong> ${data.oxygenLevel.toFixed(
				1
			)}%</div>`;
		}

		if (data.actionClass !== undefined && data.actionClass >= 0) {
			const activityInfo = dataHandler.getActivityInfo(data.actionClass);
			html += `<div class="data-info"><strong>Activity:</strong> ${
				data.activityName || activityInfo.name
			}</div>`;
			if (data.confidence) {
				html += `<div class="data-info"><strong>Confidence:</strong> ${(
					data.confidence * 100
				).toFixed(1)}%</div>`;
			}
		}

		if (data.timestamp) {
			html += `<div class="data-info"><strong>Timestamp:</strong> ${data.timestamp}</div>`;
		}

		if (html === '') {
			html = 'No data available';
		}

		this.elements.lastDataDiv.innerHTML = html;

		// Update the "last updated" time
		this.elements.lastDataTime.textContent = `Updated: ${now.toLocaleTimeString()}`;
	}

	/**
	 * Render the data history in the UI
	 */
	renderDataHistory(dataHistory) {
		if (!dataHistory || dataHistory.length === 0) {
			this.elements.dataHistoryContainer.innerHTML =
				'<div class="history-empty">No data available yet</div>';
			return;
		}

		let html = '';

		dataHistory.forEach((item, index) => {
			const timeString = new Date(
				item.historyTimestamp
			).toLocaleTimeString();
			const activityInfo = dataHandler.getActivityInfo(item.actionClass);

			html += `
                <div class="history-item ${index === 0 ? 'new' : ''}">
                    <div class="history-data">
                        <strong>${activityInfo.name}</strong> - 
                        HR: ${item.heartRate?.toFixed(1) || '--'} BPM, 
                        SpO₂: ${item.oxygenLevel?.toFixed(1) || '--'}%
                        ${
							item.confidence
								? ` (Conf: ${(item.confidence * 100).toFixed(
										0
								  )}%)`
								: ''
						}
                    </div>
                    <div class="history-time">${timeString}</div>
                </div>
            `;
		});

		this.elements.dataHistoryContainer.innerHTML = html;
	}
}

// Create global instance
const uiController = new UIController();
