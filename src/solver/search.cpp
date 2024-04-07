#include "search.h"

#include <algorithm>
#include <cassert>

#include "position.h"
#include "settings.h"
#include "table.h"

static constexpr int INF_SCORE = 10000;

static NodeType get_node_type(int value, int alpha, int beta) {
    if (value <= alpha) {
        return NodeType::UPPER;
    }

    if (value >= beta) {
        return NodeType::LOWER;
    }

    return NodeType::EXACT;
}

static int count_bits(board b) {
    int result;
    for (result = 0; b; result++) {
        b &= b - 1;
    }

    return result;
}

static float heuristic(const Position &pos, board opponent_threats, int col, bool is_table_move) {
    // Opponent and player are swapped, since a move was just played.
    board player_threats = pos.find_opponent_threats();
    board threats = pos.find_useful_threats(player_threats, opponent_threats);

    float center_score = (float)std::min(col, BOARD_WIDTH - col - 1) / BOARD_WIDTH;

    // clang-format off
    return 1.2f * count_bits(pos.find_next_turn_threats(threats))
        + 0.5f * is_table_move
        + 0.5f * count_bits(pos.find_next_next_turn_threats(threats))
        + 0.3f * count_bits(threats)
        + 0.1f * center_score;
    // clang-format on
}

void Search::sort_moves(Position &pos, Node *children, board opponent_threats,
        int num_moves, int *moves, int score_jitter, int table_move) {
    assert(num_moves > 0);
    assert(score_jitter >= 0);
    assert(table_move == -1 || (0 <= table_move && table_move < BOARD_WIDTH));

    // Score each valid move by taking a guess on how good the position will be for the player.
    for (int i = 0; i < num_moves; i++) {
        int col = moves[i];

        board before_move = pos.move(col);
        children[col].score = heuristic(pos, opponent_threats, col, col == table_move);
        pos.unmove(before_move);

        // Add some noise to move scores to help threads desync.
        if (score_jitter > 0) {
            int max_rand = 1 + (score_jitter % BOARD_WIDTH);
            children[col].score += MOVE_SCORE_JITTER * (dist(rand) % max_rand);
        }
    }

    // Sort moves according to score, high to low.
    std::sort(moves, moves + num_moves, [&children](int a, int b) { return children[a].score > children[b].score; });
}

int Search::search(Position &pos, int alpha, int beta, int score_jitter) {
    assert(alpha < beta);
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());
    assert(!pos.wins_this_move(pos.find_player_threats()));

    Node child(pos);
    bool is_static = false;

    int child_score = static_search(child, alpha, beta, is_static);
    if (is_static) {
        return child_score;
    }

    if (pos.is_same_player(child.pos)) {
        return negamax(child, alpha, beta, score_jitter);
    } else {
        return -negamax(child, -beta, -alpha, score_jitter);
    }
}

int Search::negamax(Node &node, int alpha, int beta, int score_jitter) {
    assert(alpha < beta);
    assert(!node.pos.is_game_over());
    assert(!node.pos.wins_this_move(node.pos.find_player_threats()));

    stats->new_node();

    // If another thread found the result we are looking for,
    // immediately return.
    if (stop_search) {
        return SEARCH_STOPPED;
    }

    int original_alpha = alpha;
    int original_beta = beta;

    // Prefetch the position's entry.
    if (!node.did_lookup) {
        node.hash = node.pos.hash(node.is_mirrored);
        table.prefetch(node.hash);
    }

    // If there are too few empty spaces left on the board for the player to win, then the best
    // score possible is a draw.
    if (!node.pos.can_player_win()) {
        beta = std::min(beta, 0);
    }

    // This function will never be called on a position that can be statically evaluated, so we
    // know it is not possible to win or lose in the next two turns, so tighten bounds.
    alpha = std::max(alpha, node.pos.score_loss(2));
    beta = std::min(beta, node.pos.score_win(2));
    if (alpha >= beta) {
        return beta;
    }

    // Find the opponents threats, and any moves directly below a threat.
    // These moves will not be played.
    board opponent_threats = node.pos.find_opponent_threats();
    board non_losing_moves = node.pos.find_non_losing_moves(opponent_threats);

    int value = -INF_SCORE;

    int num_moves = 0;
    int moves[BOARD_WIDTH];
    Node children[BOARD_WIDTH];

    // Next, we will test each move if it can be statically evaluated (i.e. only
    // playing forced moves will lead to a forced win, loss, or draw). Moves that
    // are statically evaluated will not be recursed into, and can be used to
    // tighten search bounds.
    //
    // Moves which cannot be statically evaluated will instead be assigned a score
    // which is a guess of how good the move is. Moves with the highest score will
    // be searched first.
    for (int col = 0; col < BOARD_WIDTH; col++) {
        if (node.pos.is_non_losing_move(non_losing_moves, col)) {
            bool is_static = false;

            children[col] = Node(node.pos);
            children[col].pos.move(col);
            int child_alpha = -static_search(children[col], -beta, -alpha, is_static);

            alpha = std::max(alpha, child_alpha);
            value = std::max(value, child_alpha);

            if (alpha >= beta) {
                return alpha;
            }

            if (!is_static) {
                moves[num_moves] = col;
                num_moves++;
            }
        }
    }

    // At this point we know it is not possible to win in the next four turns, so tighten
    // bounds further.
    beta = std::min(beta, node.pos.score_win(4));
    if (alpha >= beta) {
        return beta;
    }

    // If every move was statically evaluated, then there is nothing more to do.
    if (num_moves == 0) {
        return value;
    }

    // Check if this state has already been seen.
    int table_move = -1;
    if (!node.did_lookup) {
        node.entry = table.get(node.hash);
    }
    switch (node.entry.get_type()) {
        case NodeType::MISS:
            break;

        case NodeType::EXACT:
            return node.entry.get_score();

        case NodeType::LOWER:
            alpha = std::max(alpha, node.entry.get_score());
            table_move = node.entry.get_move(node.is_mirrored);
            break;

        case NodeType::UPPER:
            beta = std::min(beta, node.entry.get_score());
            break;
    }

    if (alpha >= beta) {
        return node.entry.get_score();
    }

    // Sort moves according to score.
    sort_moves(node.pos, children, opponent_threats, num_moves, moves, score_jitter, table_move);

    // If none of the above checks pass, then this is an internal node and we must
    // evaluate the child nodes to determine the score of this node.
    int best_recursion_value = -INF_SCORE, best_move_index = -1, best_move_col = -1;
    for (int i = 0; i < num_moves && alpha < beta; i++) {
        int col = moves[i];

        int child_score_jitter = score_jitter / BOARD_WIDTH;

        // If the difference in score between this move and the next & previous moves is too
        // large to be affected by score jitter, then pass the move jitter on to the child.
        if ((i == 0 || children[moves[i - 1]].score > children[col].score + MOVE_SCORE_JITTER) &&
            (i == num_moves - 1 || children[moves[i + 1]].score < children[col].score - MOVE_SCORE_JITTER)) {
            child_score_jitter = score_jitter;
        }

        // The children of this node can be more than one move deeper if static
        // evalulation found and played forced moves.
        int child_score = node.pos.is_same_player(children[col].pos)
                              ? negamax(children[col], alpha, beta, child_score_jitter)
                              : -negamax(children[col], -beta, -alpha, child_score_jitter);

        // If the child aborted the search, propagate the signal upwards.
        if (abs(child_score) == SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        // If the current move is the best move we have found so far.
        if (child_score > best_recursion_value) {
            best_move_index = i;
            best_move_col = col;
            best_recursion_value = child_score;

            value = std::max(value, child_score);
            alpha = std::max(alpha, child_score);
        }
    }

    assert(best_recursion_value != -INF_SCORE);
    assert(best_move_col != -1);
    assert(best_move_index != -1);
    assert(alpha >= value);
    assert(value > -INF_SCORE);

    // Store the result in the transposition table.
    NodeType type = get_node_type(value, original_alpha, original_beta);
    table.put(node.hash, node.is_mirrored, best_move_col, type, value);

    // Update statistics.
    stats->new_interior_node(type, node.pos.num_moves());
    if (best_move_index == 0) {
        stats->best_move_guessed();
    } else if (best_move_index == num_moves - 1) {
        // Oops.
        stats->worst_move_guessed();
    }

    progress->completed_node(id, node.pos.num_moves());

    return value;
}

int Search::static_search(Node &node, int alpha, int beta, bool &is_static) {
    assert(alpha < beta);
    assert(!is_static);
    assert(!node.pos.is_game_over());
    assert(!node.pos.wins_this_move(node.pos.find_player_threats()));

    // If there are too few empty spaces left on the board for the player to win, then the best
    // score possible is a draw.
    if (!node.pos.can_player_win()) {
        beta = std::min(beta, 0);

        if (alpha >= beta) {
            is_static = true;
            return beta;
        }
    }

    // Find the opponents threats, and any moves directly below a threat.
    // These moves will not be played.
    board opponent_threats = node.pos.find_opponent_threats();
    board non_losing_moves = node.pos.find_non_losing_moves(opponent_threats);
    board opponent_wins = node.pos.wins_this_move(opponent_threats);

    // Check if the opponent can force a win next move.
    if (node.pos.is_forced_loss_next_turn(opponent_wins, non_losing_moves)) {
        is_static = true;
        return node.pos.score_loss();
    }

    // At this point we know it is not possible to win or lose in the next two turns, so tighten bounds.
    alpha = std::max(alpha, node.pos.score_loss(2));
    beta = std::min(beta, node.pos.score_win(2));
    if (alpha >= beta) {
        is_static = true;
        return alpha;
    }

    // Check if we have a forced move and if so, statically evaluate it.
    board forced_move = node.pos.find_forced_move(opponent_wins, non_losing_moves);
    if (forced_move) {
        node.pos.move(forced_move);
        int child_score = -static_search(node, -beta, -alpha, is_static);

        if (is_static) {
            return child_score;
        }

        alpha = std::max(alpha, child_score);
        if (alpha >= beta) {
            is_static = true;
            return alpha;
        }
    }

    // If we do not have a forced move then this position cannot be statically evaluated.
    // Do a table lookup to see if we can tighten search bounds.
    else if (node.pos.num_moves() < ENHANCED_TABLE_CUTOFF_PLIES) {
        node.did_lookup = true;
        node.hash = node.pos.hash(node.is_mirrored);

        // Check if this state has already been seen.
        node.entry = table.get(node.hash);

        switch (node.entry.get_type()) {
            case NodeType::MISS:
                break;

            case NodeType::EXACT:
                is_static = true;
                return node.entry.get_score();

            case NodeType::LOWER:
                alpha = std::max(alpha, node.entry.get_score());
                if (alpha >= beta) {
                    is_static = true;
                    return node.entry.get_score();
                }
                break;

            case NodeType::UPPER:
                beta = std::min(beta, node.entry.get_score());
                if (alpha >= beta) {
                    is_static = true;
                    return node.entry.get_score();
                }
                return beta;
        }
    }

    return INF_SCORE;
}
