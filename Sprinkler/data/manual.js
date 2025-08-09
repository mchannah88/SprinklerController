document.addEventListener('DOMContentLoaded', function() {
    const controlsContainer = document.getElementById('manual-controls-container');
    const stopAllButton = document.getElementById('stop-all-button');
    const statusMessage = document.getElementById('status-message');
    const numZones = 5;

    // Create controls for each zone
    for (let i = 1; i <= numZones; i++) {
        const zoneDiv = document.createElement('div');
        zoneDiv.className = 'zone-manual';
        zoneDiv.innerHTML = `
            <h2>Zone ${i}</h2>
            <div class="form-group">
                <label>Duration (minutes):</label>
                <input type="number" class="duration" min="1" value="10">
            </div>
            <button class="start-button" data-zone="${i}">Start</button>
        `;
        controlsContainer.appendChild(zoneDiv);
    }

    // Add event listeners to all "Start" buttons
    document.querySelectorAll('.start-button').forEach(button => {
        button.addEventListener('click', startZone);
    });

    // Event listener for the "Stop All" button
    stopAllButton.addEventListener('click', stopAllZones);

    async function startZone(event) {
        const zoneId = event.target.dataset.zone;
        const durationInput = document.querySelector(`.zone-manual:nth-child(${zoneId}) .duration`);
        const duration = parseInt(durationInput.value);

        if (duration > 0) {
            try {
                statusMessage.textContent = `Starting Zone ${zoneId}...`;
                statusMessage.style.color = 'orange';

                const response = await fetch(`/manual-start?zone=${zoneId}&duration=${duration}`);
                const result = await response.json();

                if (result.status === 'success') {
                    statusMessage.textContent = `Zone ${zoneId} is ON for ${duration} minutes.`;
                    statusMessage.style.color = 'green';
                } else {
                    statusMessage.textContent = `Error: ${result.message}`;
                    statusMessage.style.color = 'red';
                }
            } catch (error) {
                statusMessage.textContent = 'Failed to start zone. Check connection.';
                statusMessage.style.color = 'red';
            }
        } else {
            statusMessage.textContent = 'Please enter a valid duration.';
            statusMessage.style.color = 'red';
        }
    }

    async function stopAllZones() {
        try {
            statusMessage.textContent = 'Stopping all zones...';
            statusMessage.style.color = 'orange';

            const response = await fetch('/manual-stop');
            const result = await response.json();

            if (result.status === 'success') {
                statusMessage.textContent = 'All zones have been turned OFF.';
                statusMessage.style.color = 'green';
            } else {
                statusMessage.textContent = `Error: ${result.message}`;
                statusMessage.style.color = 'red';
            }
        } catch (error) {
            statusMessage.textContent = 'Failed to stop zones. Check connection.';
            statusMessage.style.color = 'red';
        }
    }
});