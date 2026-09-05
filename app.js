
document.addEventListener('DOMContentLoaded', () => {
    // API Endpoints
    const API_BASE = '/api';

    // Application State
    let participants = [];
    let stats = null;
    let mainChartInstance = null;
    let currentChartView = 'overall';

    // Mode Switcher & Panels
    const modeBtns = document.querySelectorAll('.mode-btn');
    const viewPanels = document.querySelectorAll('.view-panel');

    // Header Metrics
    const statTotal = document.getElementById('stat-total');
    const statPresent = document.getElementById('stat-present');
    const statAbsent = document.getElementById('stat-absent');
    const statRate = document.getElementById('stat-rate');

    // Gate Verification Search
    const verifyForm = document.getElementById('verify-form');
    const verifyQueryInput = document.getElementById('verify-query');
    const verifyResultContainer = document.getElementById('verify-result');

    // Directory Table & Filters
    const dirSearch = document.getElementById('dir-search');
    const dirFilterBranch = document.getElementById('dir-filter-branch');
    const dirFilterStatus = document.getElementById('dir-filter-status');
    const directoryTableBody = document.getElementById('directory-table-body');
    
    // Registration Modal
    const btnOpenAdd = document.getElementById('btn-open-add');
    const btnCloseModal = document.getElementById('btn-close-modal');
    const btnCancelModal = document.getElementById('btn-cancel-modal');
    const addModal = document.getElementById('add-modal');
    const addParticipantForm = document.getElementById('add-participant-form');

    // Analytics Switcher
    const chartOptBtns = document.querySelectorAll('.chart-opt-btn');
    const chartTitle = document.getElementById('chart-title');
    const chartSubtitle = document.getElementById('chart-subtitle');

    // Toast Notification
    const toast = document.getElementById('toast');
    const toastMessage = document.getElementById('toast-message');

    // INITIALIZATION
    init();

    async function init() {
        setupModeSwitcher();
        setupEventListeners();
        await refreshAllData();
    }

    // MODE SWITCHER
    function setupModeSwitcher() {
        modeBtns.forEach(btn => {
            btn.addEventListener('click', () => {
                const targetView = btn.getAttribute('data-view');
                
                modeBtns.forEach(b => b.classList.remove('active'));
                viewPanels.forEach(p => p.classList.remove('active'));

                btn.classList.add('active');
                const activePanel = document.getElementById(`view-${targetView}`);
                if (activePanel) {
                    activePanel.classList.add('active');
                }

                if (targetView === 'analytics') {
                    renderSelectedChart();
                }
            });
        });
    }

    // EVENT LISTENERS
    function setupEventListeners() {
        // Verification Search Form Submit
        verifyForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const query = verifyQueryInput.value.trim();
            if (!query) return;
            await searchAndDisplayVerification(query);
        });

        // Directory Live Filters
        dirSearch.addEventListener('input', renderDirectoryTable);
        dirFilterBranch.addEventListener('change', renderDirectoryTable);
        dirFilterStatus.addEventListener('change', renderDirectoryTable);

        // Modal Open / Close
        btnOpenAdd.addEventListener('click', () => addModal.classList.remove('hidden'));
        btnCloseModal.addEventListener('click', () => addModal.classList.add('hidden'));
        btnCancelModal.addEventListener('click', () => addModal.classList.add('hidden'));

        // Analytics Sub-Chart View Buttons
        chartOptBtns.forEach(btn => {
            btn.addEventListener('click', () => {
                chartOptBtns.forEach(b => b.classList.remove('active'));
                btn.classList.add('active');
                currentChartView = btn.getAttribute('data-chart');
                renderSelectedChart();
            });
        });

        // Add Participant Form Submit
        addParticipantForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const newStudent = {
                collegeId: document.getElementById('add-id').value.trim(),
                name: document.getElementById('add-name').value.trim(),
                email: document.getElementById('add-email').value.trim(),
                phone: document.getElementById('add-phone').value.trim(),
                branch: document.getElementById('add-branch').value,
                year: document.getElementById('add-year').value
            };

            try {
                const res = await fetch(`${API_BASE}/add`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(newStudent)
                });
                const data = await res.json();
                if (data.success) {
                    showToast('Participant registered successfully!', 'success');
                    addModal.classList.add('hidden');
                    addParticipantForm.reset();
                    await refreshAllData();
                } else {
                    showToast(data.message || 'Failed to add participant', 'error');
                }
            } catch (err) {
                showToast('Error communicating with backend server.', 'error');
            }
        });
    }

    // DATA REFRESH FROM C++ REST API
    async function refreshAllData() {
        try {
            const [pRes, sRes] = await Promise.all([
                fetch(`${API_BASE}/participants`),
                fetch(`${API_BASE}/stats`)
            ]);

            participants = await pRes.json();
            stats = await sRes.json();

            updateHeaderStats();
            renderDirectoryTable();
            
            const activePanel = document.querySelector('.view-panel.active');
            if (activePanel && activePanel.id === 'view-analytics') {
                renderSelectedChart();
            }
        } catch (err) {
            console.error('Failed to load data from C++ server:', err);
        }
    }

    // UPDATE TOP HEADER METRICS
    function updateHeaderStats() {
        if (!stats) return;
        statTotal.textContent = stats.totalRegistered || 0;
        statPresent.textContent = stats.totalPresent || 0;
        statAbsent.textContent = stats.totalAbsent || 0;
        statRate.textContent = (stats.attendancePercentage || 0).toFixed(1) + '%';
    }

    // ENTRANCE GATE VERIFICATION
    async function searchAndDisplayVerification(query) {
        const lowerQ = query.toLowerCase();
        const found = participants.find(p => 
            p.collegeId.toLowerCase() === lowerQ ||
            p.email.toLowerCase() === lowerQ ||
            p.name.toLowerCase() === lowerQ ||
            p.phone === query
        );

        verifyResultContainer.classList.remove('hidden');

        if (found) {
            renderStudentCard(found);
        } else {
            verifyResultContainer.innerHTML = `
                <div class="student-card not-found">
                    <div>
                        <h4 style="color:#e11d48; font-size:1.1rem;">Participant Not Found</h4>
                        <p style="color:#475569; font-size:0.9rem; margin-top:0.3rem;">No registration found matching query: <strong>"${escapeHtml(query)}"</strong>.</p>
                        <p style="color:#64748b; font-size:0.8rem; margin-top:0.5rem;">Please check for typos or register the student under Participant Directory.</p>
                    </div>
                </div>
            `;
        }
    }

    function renderStudentCard(p) {
        const isPresent = p.isPresent;
        verifyResultContainer.innerHTML = `
            <div class="student-card ${isPresent ? 'is-present' : 'is-absent'}">
                <div style="flex:1;">
                    <div style="display:flex; align-items:center; gap:0.75rem;">
                        <h3 style="font-size:1.25rem;">${escapeHtml(p.name)}</h3>
                        <span class="badge ${isPresent ? 'badge-success' : 'badge-warning'}">
                            ${isPresent ? 'Present (Verified)' : 'Not Yet Marked'}
                        </span>
                    </div>

                    <div class="student-info-grid">
                        <div class="info-item">
                            <label>College ID</label>
                            <span>${escapeHtml(p.collegeId)}</span>
                        </div>
                        <div class="info-item">
                            <label>Branch & Year</label>
                            <span>${escapeHtml(p.branch)} - ${escapeHtml(p.year)}</span>
                        </div>
                        <div class="info-item">
                            <label>Email ID</label>
                            <span>${escapeHtml(p.email)}</span>
                        </div>
                        <div class="info-item">
                            <label>Check-in Time</label>
                            <span>${p.checkInTime ? escapeHtml(p.checkInTime) : '—'}</span>
                        </div>
                    </div>
                </div>

                <div>
                    ${!isPresent ? `
                        <button class="btn btn-success btn-mark-present" data-id="${escapeHtml(p.collegeId)}">
                            Mark Present
                        </button>
                    ` : `
                        <button class="btn btn-outline" disabled style="opacity:0.75; cursor:default;">
                            Verified Entry
                        </button>
                    `}
                </div>
            </div>
        `;

        const btnMark = verifyResultContainer.querySelector('.btn-mark-present');
        if (btnMark) {
            btnMark.addEventListener('click', () => markAttendance(p.collegeId));
        }
    }

    async function markAttendance(id) {
        try {
            const res = await fetch(`${API_BASE}/mark`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ id: id })
            });

            const data = await res.json();
            if (data.success) {
                showToast(`Attendance marked for ${data.participant.name}!`, 'success');
                await refreshAllData();
                renderStudentCard(data.participant);
            } else {
                showToast(data.message || 'Error marking attendance', 'error');
            }
        } catch (err) {
            showToast('Failed to connect to C++ backend server.', 'error');
        }
    }

    // PARTICIPANT DIRECTORY TABLE
    function renderDirectoryTable() {
        const q = dirSearch.value.trim().toLowerCase();
        const branchFilter = dirFilterBranch.value;
        const statusFilter = dirFilterStatus.value;

        const filtered = participants.filter(p => {
            const matchesQ = p.name.toLowerCase().includes(q) ||
                             p.collegeId.toLowerCase().includes(q) ||
                             p.email.toLowerCase().includes(q);
            
            const matchesBranch = (branchFilter === 'ALL' || p.branch === branchFilter);
            
            const matchesStatus = (statusFilter === 'ALL') ||
                                  (statusFilter === 'PRESENT' && p.isPresent) ||
                                  (statusFilter === 'ABSENT' && !p.isPresent);

            return matchesQ && matchesBranch && matchesStatus;
        });

        if (filtered.length === 0) {
            directoryTableBody.innerHTML = `
                <tr>
                    <td colspan="7" style="text-align:center; padding:2rem; color:#64748b;">
                        No matching participant records found.
                    </td>
                </tr>
            `;
            return;
        }

        directoryTableBody.innerHTML = filtered.map(p => `
            <tr>
                <td><strong>${escapeHtml(p.collegeId)}</strong></td>
                <td>${escapeHtml(p.name)}</td>
                <td>
                    <div style="font-size:0.85rem;">${escapeHtml(p.email)}</div>
                    <div style="font-size:0.78rem; color:#64748b;">${escapeHtml(p.phone)}</div>
                </td>
                <td><span class="badge" style="background:#f1f5f9; color:#334155;">${escapeHtml(p.branch)} (${escapeHtml(p.year)})</span></td>
                <td>
                    <span class="badge ${p.isPresent ? 'badge-success' : 'badge-warning'}">
                        ${p.isPresent ? 'Present' : 'Not Marked'}
                    </span>
                </td>
                <td style="font-size:0.82rem; color:#64748b;">${p.checkInTime ? escapeHtml(p.checkInTime) : '—'}</td>
                <td style="text-align: right;">
                    ${!p.isPresent ? `
                        <button class="btn btn-primary btn-sm-mark" data-id="${escapeHtml(p.collegeId)}" style="padding:0.35rem 0.75rem; font-size:0.8rem;">
                            Mark Present
                        </button>
                    ` : `<span style="color:#059669; font-size:0.85rem; font-weight:600;">Verified</span>`}
                </td>
            </tr>
        `).join('');

        directoryTableBody.querySelectorAll('.btn-sm-mark').forEach(btn => {
            btn.addEventListener('click', () => {
                const id = btn.getAttribute('data-id');
                markAttendance(id);
            });
        });
    }

    // ANALYTICS DASHBOARD CHART RENDERER
    function renderSelectedChart() {
        if (!stats) return;

        const ctx = document.getElementById('main-analytics-chart').getContext('2d');
        if (mainChartInstance) {
            mainChartInstance.destroy();
        }

        if (currentChartView === 'overall') {
            chartTitle.textContent = 'Overall Attendance Breakdown';
            chartSubtitle.textContent = 'Ratio of verified present participants versus remaining unverified registrations.';

            mainChartInstance = new Chart(ctx, {
                type: 'doughnut',
                data: {
                    labels: ['Present', 'Not Yet Marked'],
                    datasets: [{
                        data: [stats.totalPresent, stats.totalAbsent],
                        backgroundColor: ['#059669', '#cbd5e1'],
                        borderWidth: 3,
                        borderColor: '#ffffff'
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { position: 'bottom', labels: { padding: 20, font: { size: 13, weight: '500' } } }
                    }
                }
            });

        } else if (currentChartView === 'year') {
            chartTitle.textContent = 'Year-Wise Attendance Statistics';
            chartSubtitle.textContent = 'Comparison of present students vs total registered across academic years.';

            const years = ['1st Year', '2nd Year', '3rd Year', '4th Year'];
            const yearPresentData = years.map(y => stats.yearStats[y] ? stats.yearStats[y].present : 0);
            const yearTotalData = years.map(y => stats.yearStats[y] ? stats.yearStats[y].total : 0);

            mainChartInstance = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: years,
                    datasets: [
                        {
                            label: 'Present Students',
                            data: yearPresentData,
                            backgroundColor: '#2563eb',
                            borderRadius: 6
                        },
                        {
                            label: 'Total Registered',
                            data: yearTotalData,
                            backgroundColor: '#e2e8f0',
                            borderRadius: 6
                        }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { position: 'top' }
                    },
                    scales: {
                        y: { beginAtZero: true, ticks: { precision: 0 } }
                    }
                }
            });

        } else if (currentChartView === 'branch') {
            chartTitle.textContent = 'Branch-Wise Attendance Statistics';
            chartSubtitle.textContent = 'Comparison of present students vs total registered across academic branches.';

            const branches = ['CSE', 'ECE', 'ME', 'IT', 'CE'];
            const branchPresentData = branches.map(b => stats.branchStats[b] ? stats.branchStats[b].present : 0);
            const branchTotalData = branches.map(b => stats.branchStats[b] ? stats.branchStats[b].total : 0);

            mainChartInstance = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: branches,
                    datasets: [
                        {
                            label: 'Present Students',
                            data: branchPresentData,
                            backgroundColor: '#10b981',
                            borderRadius: 6
                        },
                        {
                            label: 'Total Registered',
                            data: branchTotalData,
                            backgroundColor: '#cbd5e1',
                            borderRadius: 6
                        }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { position: 'top' }
                    },
                    scales: {
                        y: { beginAtZero: true, ticks: { precision: 0 } }
                    }
                }
            });
        }
    }

    // TOAST NOTIFICATION
    function showToast(message, type = 'success') {
        toastMessage.textContent = message;
        toast.classList.remove('hidden');
        setTimeout(() => toast.classList.add('hidden'), 3500);
    }

    // HELPER: Escape HTML
    function escapeHtml(str) {
        if (!str) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#039;');
    }
});
