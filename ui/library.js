addToLibrary({
    solve_callback: (score, bestMove, movesLeft) => {
        const bc = new BroadcastChannel("solve_events");
        bc.postMessage({ type: "solved", score, bestMove, movesLeft });
    },

    win_callback: (score) => {
        const bc = new BroadcastChannel("solve_events");
        bc.postMessage({ type: "won", score });
    },

    cancelled_callback: () => {
        const bc = new BroadcastChannel("solve_events");
        bc.postMessage({ type: "cancelled" });
    },
});
