/**
 * Main Application - Connects data handler and UI controller
 */

class App {
	constructor() {
		// State
		this.state = {
			isConnected: false,
			isCollecting: initialState.isCollecting,
			deviceCurrentState: 'Unknown',
			lastStatusCheck: 0,
			statusCheckInProgress: false,
			commandInProgress: false,
			lastCommandSent: '',
			lastCommandStatus: '',
			lastCommandTime: 0,
			lastStateUpdateTime: 0,
		};

		// Initialize
		this.init();
	}

	/**
	 * Initialize the application
	 */
	init() {
		// Set up event listeners for buttons
		this.setupEventListeners();

		// Update UI initially
		uiController.updateUI(this.state);

		// Start periodic status checks
		setInterval(() => this.checkStatus(), 2000);

		// Check status immediately
		this.checkStatus();
	}

	/**
	 * Set up event listeners for all buttons
	 */
	setupEventListeners() {
		// Start button
		uiController.elements.startButton.addEventListener(
			'click',
			async () => {
				await this.startCollection();
			}
		);

		// Stop button
		uiController.elements.stopButton.addEventListener('click', async () => {
			await this.stopCollection();
		});

		// Clear connection button
		uiController.elements.clearConnectionButton.addEventListener(
			'click',
			async () => {
				await this.clearConnection();
			}
		);

		// Check status button
		uiController.elements.checkStatusButton.addEventListener(
			'click',
			async () => {
				await this.checkDeviceStatus();
			}
		);
	}

	/**
	 * Start data collection
	 */
	async startCollection() {
		try {
			// Immediate UI feedback
			this.state.commandInProgress = true;
			uiController.updateUI(this.state);
			uiController.showCommandStatus('START', 'pending');

			const response = await fetch('/start', { method: 'POST' });
			const data = await response.json();

			if (data.status === 'success') {
				uiController.showCommandStatus('START', 'success');
				uiController.showToast(
					'Started data collection successfully',
					'success'
				);
				this.state.isCollecting = true;

				// Display "started collection" message
				uiController.elements.lastDataDiv.innerHTML =
					'<div style="color: green; font-weight: bold;">Started data collection. Waiting for data...</div>';
			} else if (data.status === 'pending') {
				// Command sent but waiting for confirmation from sensor
				uiController.showCommandStatus(
					'START',
					'pending',
					data.message
				);
				uiController.showToast(
					'Waiting for sensor confirmation...',
					'info'
				);

				// Keep checking status until we get confirmation
				let checkAttempts = 0;
				const checkInterval = setInterval(async () => {
					checkAttempts++;
					await this.checkStatus(true); // Force a status check

					// If state changed to collecting or we've tried enough times, stop checking
					if (this.state.isCollecting || checkAttempts >= 10) {
						clearInterval(checkInterval);
						if (this.state.isCollecting) {
							uiController.showCommandStatus('START', 'success');
							uiController.showToast(
								'Sensor confirmed collection started',
								'success'
							);
						} else {
							uiController.showCommandStatus(
								'START',
								'error',
								'Sensor did not confirm state change'
							);
							uiController.showToast(
								'Sensor did not start collecting',
								'error'
							);
						}
						this.state.commandInProgress = false;
						uiController.updateUI(this.state);
					}
				}, 500);
			} else {
				uiController.showCommandStatus('START', 'error', data.message);
				await this.checkStatus(true); // Force a status check to ensure UI matches reality
			}
		} catch (error) {
			console.error('Error:', error);
			uiController.showCommandStatus('START', 'error', error.message);
			uiController.showToast(
				`Error starting collection: ${error.message}`,
				'error'
			);
			await this.checkStatus(true); // Force a status check to ensure UI matches reality
		} finally {
			this.state.commandInProgress = false;
			uiController.updateUI(this.state);
		}
	}

	/**
	 * Stop data collection
	 */
	async stopCollection() {
		try {
			// Immediate UI feedback
			this.state.commandInProgress = true;
			uiController.updateUI(this.state);
			uiController.showCommandStatus('STOP', 'pending');

			const response = await fetch('/stop', { method: 'POST' });
			const data = await response.json();

			if (data.status === 'success') {
				uiController.showCommandStatus('STOP', 'success');
				uiController.showToast('Stopped data collection', 'success');
				this.state.isCollecting = false;

				// Add stopped indicator
				uiController.elements.lastDataDiv.innerHTML +=
					'<div style="color: orange; font-weight: bold; margin-top: 10px;">Data collection stopped</div>';
			} else if (data.status === 'pending') {
				// Command sent but waiting for confirmation from sensor
				uiController.showCommandStatus('STOP', 'pending', data.message);
				uiController.showToast(
					'Waiting for sensor confirmation...',
					'info'
				);

				// Keep checking status until we get confirmation
				let checkAttempts = 0;
				const checkInterval = setInterval(async () => {
					checkAttempts++;
					await this.checkStatus(true); // Force a status check

					// If state changed to not collecting or we've tried enough times, stop checking
					if (!this.state.isCollecting || checkAttempts >= 10) {
						clearInterval(checkInterval);
						if (!this.state.isCollecting) {
							uiController.showCommandStatus('STOP', 'success');
							uiController.showToast(
								'Sensor confirmed collection stopped',
								'success'
							);
						} else {
							uiController.showCommandStatus(
								'STOP',
								'error',
								'Sensor did not confirm state change'
							);
							uiController.showToast(
								'Sensor is still collecting',
								'error'
							);
						}
						this.state.commandInProgress = false;
						uiController.updateUI(this.state);
					}
				}, 500);
			} else {
				uiController.showCommandStatus('STOP', 'error', data.message);
				await this.checkStatus(true); // Force a status check to ensure UI matches reality
			}
		} catch (error) {
			console.error('Error:', error);
			uiController.showCommandStatus('STOP', 'error', error.message);
			uiController.showToast(
				`Error stopping collection: ${error.message}`,
				'error'
			);
			await this.checkStatus(true); // Force a status check to ensure UI matches reality
		} finally {
			this.state.commandInProgress = false;
			uiController.updateUI(this.state);
		}
	}

	/**
	 * Clear connection
	 */
	async clearConnection() {
		try {
			const response = await fetch('/clear-connection', {
				method: 'POST',
			});
			const data = await response.json();

			if (data.status === 'success') {
				uiController.showToast(
					'Connection cleared. Waiting for new sensor connection.',
					'info'
				);
				this.state.isConnected = false;
				this.state.isCollecting = false;
				uiController.elements.deviceStatusDiv.style.display = 'none';
				uiController.updateUI(this.state);
			} else {
				uiController.showToast(
					`Failed to clear connection: ${data.message}`,
					'error'
				);
			}
		} catch (error) {
			console.error('Error:', error);
			uiController.showToast(
				`Error clearing connection: ${error.message}`,
				'error'
			);
		}
	}

	/**
	 * Check device status
	 */
	async checkDeviceStatus() {
		try {
			uiController.elements.deviceStatusDiv.textContent =
				'Checking status...';
			uiController.elements.deviceStatusDiv.style.display = 'block';

			const response = await fetch('/check-status', { method: 'POST' });
			const data = await response.json();

			if (data.status === 'success') {
				// Show the device status
				uiController.elements.deviceStatusDiv.textContent =
					data.device_status || 'No status information available';

				// Also show as a toast for easier visibility
				uiController.showToast('Status check completed', 'info', 3000);
			} else {
				uiController.elements.deviceStatusDiv.textContent =
					'Error: ' + (data.message || 'Failed to get status');
				uiController.showToast(
					`Status check failed: ${data.message || 'Unknown error'}`,
					'error'
				);
			}
		} catch (error) {
			console.error('Error:', error);
			uiController.elements.deviceStatusDiv.textContent =
				'Error checking device status: ' + error.message;
			uiController.showToast(
				`Error checking status: ${error.message}`,
				'error'
			);
		}
	}

	/**
	 * Check status from server
	 */
	async checkStatus(forceCheck = false) {
		// Prevent multiple concurrent status checks unless forced
		if (this.state.statusCheckInProgress && !forceCheck) {
			return;
		}

		const now = Date.now();
		// Don't check more than once per second unless forced
		if (now - this.state.lastStatusCheck < 1000 && !forceCheck) {
			return;
		}

		this.state.lastStatusCheck = now;
		this.state.statusCheckInProgress = true;

		try {
			const response = await fetch('/status');
			const data = await response.json();

			const wasConnected = this.state.isConnected;
			const wasCollecting = this.state.isCollecting;
			const prevState = this.state.deviceCurrentState;

			this.state.isConnected = data.connected;

			// Always use the device's reported state as the source of truth
			this.state.isCollecting = data.collecting;
			this.state.deviceCurrentState = data.current_state || 'Unknown';
			this.state.commandInProgress = data.command_in_progress || false;

			// Check if state is PROCESSING to show appropriate UI
			const isProcessing = this.state.deviceCurrentState === 'PROCESSING';

			// If processing, treat it as collecting for UI purposes
			if (isProcessing && !this.state.isCollecting) {
				this.state.isCollecting = true;
				console.log(
					'Device is in PROCESSING state, treating as active measurement'
				);
			}

			// Update last state update time if provided
			if (data.time_since_update >= 0) {
				this.state.lastStateUpdateTime = data.time_since_update;
				uiController.updateSyncIndicator(
					this.state.lastStateUpdateTime
				);
			}

			// If connection state changed
			if (wasConnected !== this.state.isConnected) {
				console.log(
					`Connection state changed: ${wasConnected} -> ${this.state.isConnected}`
				);
				if (!this.state.isConnected) {
					// Connection lost
					uiController.elements.deviceStatusDiv.style.display =
						'block';
					uiController.elements.deviceStatusDiv.textContent =
						'Connection to device lost. Waiting for device to reconnect...';
					uiController.showToast(
						'Connection to device lost',
						'error'
					);

					// Reset states
					this.state.isCollecting = false;
					this.state.deviceCurrentState = 'DISCONNECTED';
					this.state.commandInProgress = false;
				} else {
					// Connection established
					uiController.elements.deviceStatusDiv.style.display =
						'block';
					uiController.elements.deviceStatusDiv.textContent =
						'Device connected successfully! Receiving device status...';
					uiController.showToast(
						'Device connected successfully',
						'success'
					);
				}
			}

			// If collection state changed, notify user
			if (
				wasCollecting !== this.state.isCollecting &&
				this.state.isConnected
			) {
				console.log(
					`Collection state changed from sensor: ${wasCollecting} -> ${this.state.isCollecting}`
				);

				if (this.state.isCollecting) {
					uiController.showToast(
						'Device started collecting data',
						'success'
					);
				} else {
					uiController.showToast(
						'Device stopped collecting data',
						'info'
					);
				}
			}

			// If device state changed, notify user - but not for transitions between COLLECTING and PROCESSING
			if (
				prevState !== this.state.deviceCurrentState &&
				this.state.isConnected
			) {
				console.log(
					`Device state changed: ${prevState} -> ${this.state.deviceCurrentState}`
				);

				// Only show toast for state changes that aren't just between COLLECTING and PROCESSING
				const isCollectingProcessingTransition =
					(prevState === 'COLLECTING' &&
						this.state.deviceCurrentState === 'PROCESSING') ||
					(prevState === 'PROCESSING' &&
						this.state.deviceCurrentState === 'COLLECTING');

				if (!isCollectingProcessingTransition) {
					uiController.showToast(
						`Device state changed to: ${this.state.deviceCurrentState}`,
						'info'
					);
				}
			}

			// Update last data information and add to history
			if (data.last_data) {
				const historyData = dataHandler.addToDataHistory(
					data.last_data
				);
				uiController.updateLastDataDisplay(data.last_data);
				uiController.renderDataHistory(historyData);
			}

			// Update device status if available
			if (
				data.device_status &&
				uiController.elements.deviceStatusDiv.style.display === 'block'
			) {
				uiController.elements.deviceStatusDiv.textContent =
					data.device_status || 'No device status available';
			}

			uiController.updateUI(this.state);
		} catch (error) {
			console.error('Error checking status:', error);
			uiController.elements.connectionStatus.textContent =
				'Connection error: ' + error.message;
			uiController.elements.connectionStatus.className =
				'status disconnected';
			this.state.isConnected = false;
			uiController.updateUI(this.state);
		} finally {
			this.state.statusCheckInProgress = false;
		}
	}
}

// Initialize the application when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
	const app = new App();
});
