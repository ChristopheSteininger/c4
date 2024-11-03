addToLibrary({
    // Called by the C++ side once a position has been solved.
    solve_callback: (score) => {
        const bc = new BroadcastChannel("solve_events");

        bc.postMessage({ score });
    },
});
