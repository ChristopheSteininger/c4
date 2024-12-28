const width = 7;
const height = 6;

const moves = [];
const bc = new BroadcastChannel("solve_events");

const modeElem = document.getElementById("mode");
const boardElem = document.getElementById("board");
const counterElem = document.getElementById("counter");
const evalElem = document.getElementById("eval");
const undoElem = document.getElementById("undo");
const resetElem = document.getElementById("reset");

// True if the solver is busy with a position.
var isSolving = false;

// True if the solver should solve the current position as soon it completes.
var scheduleSolve = false;

// Time when the last solve was kicked off.
var solveStartTimeMs;

// Wait for the WebAssembly module to load
Module.onRuntimeInitialized = () => {
    const position = new Module.Position();
    const solver = new Module.Solver();

    console.log(Module.Solver.get_settings_string());

    // Once the WASM solver is all setup, draw the board and enable the
    // click handlers.
    createBoard(solver, position);
    renderBoard(position);
    solve(solver, position);

    modeElem.addEventListener("change", () => solve(solver, position));
    undoElem.addEventListener("click", () => unmove(solver, position));
    resetElem.addEventListener("click", () => reset(solver, position));

    bc.onmessage = (event) => {
        const score = event.data.score;
        const runTimeMs = performance.now() - solveStartTimeMs;
        console.log(`Got ${score} from solver after ${runTimeMs} ms.`);

        isSolving = false;

        if (scheduleSolve) {
            scheduleSolve = false;
            solve(solver, position);
        } else if (isSolversTurn(position)) {
            const bestMove = solver.get_best_move(position, score);

            move(solver, position, bestMove, true);
        } else {
            showScore(position, solver, score);
        }
    };
}

function solve(solver, position) {
    // Clear the highlighted column.
    highlightColumn();

    // If we are solving a position which is now outdated, cancel the solve and
    // schedule the current position to be solved as soon as the solver has cancelled.
    if (isSolving) {
        scheduleSolve = true;
        solver.cancel();

        return;
    }

    // Otherwise, the solver is free and we can start the solve immediately.
    evalElem.innerText = "Solving position . . .";

    isSolving = true;
    solveStartTimeMs = performance.now();
    Module.solve_async(solver, position);
}

function isSolverPlaying() {
    return modeElem.value === "vs-red" || modeElem.value === "vs-yellow";
}

function isSolversTurn(position) {
    const isSolverRed = modeElem.value === "vs-red"

    return isSolverPlaying() && isSolverRed === isRedsTurn(position);
}

function move(solver, position, col, isSolverMove = false) {
    // Don't allow moves if it is the solvers turn.
    if (isSolversTurn(position) !== isSolverMove) {
        return;
    }

    if (!position.is_game_over() && position.is_move_valid(col)) {
        position.move(col);
        moves.push(col);

        renderBoard(position);
        solve(solver, position);
    }
}

function unmove(solver, position) {
    // Don't allow unmoves if it is the solvers turn.
    if (isSolversTurn(position)) {
        return;
    }

    // We also need to undo the solver's move, otherwise
    // it will auto play the same move again.
    const movesToUndo = isSolverPlaying() ? 2 : 1;

    for (let i = 0; i < movesToUndo; i++) {
        if (position.num_moves() > 0) {
            const col = moves.pop();
            position.unmove(col);
        }
    }

    renderBoard(position);
    solve(solver, position);
}

function reset(solver, position) {
    while (position.num_moves() > 0) {
        const col = moves.pop();
        position.unmove(col);
    }

    renderBoard(position);
    solve(solver, position);
}

function showScore(position, solver, score) {
    const movesLeft = position.moves_left(score);

    // The solver will return 0 moves left if the game is over.
    if (movesLeft === 0) {
        if (score > 0) {
            evalElem.innerText = `${getCurrentPlayerName(position)} won!`;
        } else if (score < 0) {
            evalElem.innerText = `${getOpponentPlayerName(position)} won!`;
        } else if (score === 0) {
            evalElem.innerText = "Draw!";
        }

        return;
    }

    // Otherwise, show the number of moves until the game is over.
    const bestMove = solver.get_best_move(position, score);
    const isFirstPlayer = (position.num_moves() & 1) == 0;

    // Don't show the best move if the solver is playing against the user.
    if (!isSolverPlaying()) {
        highlightColumn(bestMove);
    }

    if (score == 0) {
        evalElem.innerText = `Game will end in a draw.`;
    } else if (score > 0 === isFirstPlayer) {
        evalElem.innerText = `Red will win in ${movesLeft} moves.`;
    } else {
        evalElem.innerText = `Yellow will win in ${movesLeft} moves.`;
    }
}

// Create the board grid.
function createBoard(solver, position) {
    // Clear the loading text.
    boardElem.innerText = "";

    for (let col = 0; col < width; col++) {
        const colElem = document.createElement("div");
        colElem.classList.add("column");
        colElem.dataset.col = col;

        // Add the move preview to the top of the column.
        const previewElem = document.createElement("div");
        previewElem.classList.add("preview");
        previewElem.classList.add("cell");
        previewElem.dataset.col = col;

        colElem.appendChild(previewElem);

        // Add all payable positions to the column.
        for (let row = 0; row < height; row++) {
            const cellElem = document.createElement("div");
            cellElem.classList.add("playable");
            cellElem.classList.add("cell");
            cellElem.dataset.row = height - row - 1;
            cellElem.dataset.col = col;

            colElem.appendChild(cellElem);
        }

        colElem.addEventListener("click", () => move(solver, position, col, false));
        boardElem.appendChild(colElem);
    }
}

function renderBoard(position) {
    const moveNumber = position.num_moves();
    const isFirstPlayerTurn = (moveNumber % 2) == 0;

    // Update the board display to show the current state.
    boardElem.querySelectorAll(".playable").forEach(cell => {
        const col = parseInt(cell.dataset.col);
        const row = parseInt(cell.dataset.row);

        const player = position.get_player(row, col);
        if (player === -1) {
            cell.classList.add("player-1");
        } else if (player == 1) {
            cell.classList.add("player-2");
        } else {
            cell.classList.remove("player-1", "player-2");
        }
    });

    // Update the move previews at the top of the board.
    boardElem.querySelectorAll(".preview").forEach(previewElem => {
        const col = parseInt(previewElem.dataset.col);
        const isMoveValid = !position.is_game_over() && position.is_move_valid(col);

        if (!isMoveValid) {
            previewElem.classList.remove("player-1", "player-2");
        } else if (isFirstPlayerTurn) {
            previewElem.classList.add("player-1");
            previewElem.classList.remove("player-2");
        } else {
            previewElem.classList.remove("player-1");
            previewElem.classList.add("player-2");
        }
    });

    // Update the move counter.
    counterElem.innerText = `Move ${moveNumber}.`;
}

function highlightColumn(highlight) {
    boardElem.querySelectorAll('.column').forEach(colElem => {
        const col = parseInt(colElem.dataset.col);

        if (col === highlight) {
            colElem.classList.add("best-column");
        } else {
            colElem.classList.remove("best-column");
        }
    });
}

function isRedsTurn(position) {
    const moveNumber = position.num_moves();

    return (moveNumber % 2) == 0;
}

function getCurrentPlayerName(position) {
    return isRedsTurn(position) ? "Red" : "Yellow";
}

function getOpponentPlayerName(position) {
    return isRedsTurn(position) ? "Yellow" : "Red";
}
