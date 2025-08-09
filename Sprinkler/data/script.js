document.addEventListener('DOMContentLoaded', function() {
    const scheduleContainer = document.getElementById('schedule-container');
    const saveButton = document.getElementById('save-button');
    const statusMessage = document.getElementById('status-message');
    const numZones = 5;
    const days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

    // Function to create the HTML for all zones
    function createZoneInputs() {
        for (let i = 1; i <= numZones; i++) {
            const zoneDiv = document.createElement('div');
            zoneDiv.className = 'zone';
            zoneDiv.id = `zone-${i}`;

            let daysHtml = '';
            days.forEach((day, index) => {
                daysHtml += `<label><input type="checkbox" class="day" data-day="${index}"> ${day}</label>`;
            });

            zoneDiv.innerHTML = `
                <h2>Zone ${i}</h2>
                <div class="form-group">
                    <label><input type="checkbox" class="enabled"> Enabled</label>
                </div>
                <div class="form-group">
                    <label>Start Time:</label>
                    <input type="time" class="start-time">
                </div>
                <div class="form-group">
                    <label>Duration (minutes):</label>
                    <input type="number" class="duration" min="1" value="10">
                </div>
                <div class="form-group">
                    <label>Days:</label>
                    <div class="day-checkboxes">${daysHtml}</div>
                </div>
            `;
            scheduleContainer.appendChild(zoneDiv);
        }
    }

    // Function to load the schedule from the ESP32
    async function loadSchedule() {
        try {
            const response = await fetch('/schedule');
            const schedule = await response.json();

            for (let i = 1; i <= numZones; i++) {
                const zoneData = schedule[`zone${i}`];
                const zoneDiv = document.getElementById(`zone-${i}`);
                if (zoneData) {
                    zoneDiv.querySelector('.enabled').checked = zoneData.enabled;
                    zoneDiv.querySelector('.start-time').value = zoneData.startTime;
                    zoneDiv.querySelector('.duration').value = zoneData.duration;
                    zoneData.days.forEach(dayIndex => {
                        zoneDiv.querySelector(`[data-day="${dayIndex}"]`).checked = true;
                    });
                }
            }
        } catch (error) {
            console.error('Error loading schedule:', error);
            statusMessage.textContent = 'Error loading schedule.';
            statusMessage.style.color = 'red';
        }
    }

    // Function to save the schedule to the ESP32
    async function saveSchedule() {
        const schedule = {};
        for (let i = 1; i <= numZones; i++) {
            const zoneDiv = document.getElementById(`zone-${i}`);
            const daysChecked = [];
            zoneDiv.querySelectorAll('.day:checked').forEach(checkbox => {
                daysChecked.push(parseInt(checkbox.dataset.day));
            });

            schedule[`zone${i}`] = {
                enabled: zoneDiv.querySelector('.enabled').checked,
                startTime: zoneDiv.querySelector('.start-time').value,
                duration: parseInt(zoneDiv.querySelector('.duration').value),
                days: daysChecked
            };
        }

        try {
            statusMessage.textContent = 'Saving...';
            statusMessage.style.color = 'orange';
            const response = await fetch('/schedule', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(schedule)
            });
            
            const result = await response.json();

            if (result.status === 'success') {
                statusMessage.textContent = 'Settings saved successfully!';
                statusMessage.style.color = 'green';
            } else {
                statusMessage.textContent = `Error: ${result.message}`;
                statusMessage.style.color = 'red';
            }

        } catch (error) {
            console.error('Error saving schedule:', error);
            statusMessage.textContent = 'Failed to save settings. Check connection.';
            statusMessage.style.color = 'red';
        }
    }

    // Initialize the page
    createZoneInputs();
    loadSchedule();
    saveButton.addEventListener('click', saveSchedule);
});