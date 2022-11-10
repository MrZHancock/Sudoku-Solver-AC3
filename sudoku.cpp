#include <algorithm>
#include <array>
#include <bit> // requires C++ 20
#include <fstream>
#include <iostream>
#include <string>
#include <set>

// so function signatures aren't absurdly long
typedef unsigned short int USHORT;
typedef std::array<std::array<std::pair<USHORT, USHORT>, 20>, 81> CONSTRAINTS_MATRIX;
typedef std::array<unsigned short, 81> CSP;
typedef std::pair<unsigned short, unsigned short> DIFF_CONSTRAINT;


CONSTRAINTS_MATRIX generate_binary_constraints() {
    // generate an 2d of all binary constraints
    // binary_constraints[i] has every constraint on variable i
    CONSTRAINTS_MATRIX binary_constraints;
    
    // keep track of how many constraints are in each bin (i.e., the end of each bin)
    std::array<unsigned short, 81> bin_constr_counts;
    // start counting from 0
    std::fill(std::begin(bin_constr_counts), std::end(bin_constr_counts), 0U);

    // constraints for values in the same row
    for (unsigned short r = 0; r < 81; r += 9) {
        // r is the row
        for (auto c1 = r; c1 < r + 8; c1++) {
            for (auto c2 = c1 + 1; c2 < r + 9; c2++) {
                // c1 and c2 are columns in row r
                binary_constraints[c1][bin_constr_counts[c1]++] = DIFF_CONSTRAINT{c1, c2};
                binary_constraints[c2][bin_constr_counts[c2]++] = DIFF_CONSTRAINT{c1, c2};
            }
        }
    }

    // constraints for values in the same column
    for (unsigned short c = 0; c < 9; c++) {
        // c is the column
        for (auto r1 = c; r1 < 81; r1 += 9) {
            for (auto r2 = r1 + 9; r2 < 81; r2 += 9) {
                // r1 and r2 are row in column c
                binary_constraints[r1][bin_constr_counts[r1]++] = DIFF_CONSTRAINT{r1, r2};
                binary_constraints[r2][bin_constr_counts[r2]++] = DIFF_CONSTRAINT{r1, r2};
            }
        }
    }

    // constraints for values in the same sub-grid
    for (unsigned int br = 0; br < 81; br += 27) {
        for (auto bc = br; bc < br + 9; bc += 3) {
            // bc is the top-left corner of a 3x3 sub-grid
            // bc + (x/3)*9 + x%3 gives each value in the sub-grid
            // where 0 <= x < 9
            for (unsigned int i = 0; i < 8; i++) {
                auto a = bc + (i/3) * 9 + i%3;
                for (auto j = i + 1; j < 9; j++) {
                    auto b = bc + (j/3) * 9 + j%3;
                    if ((b - a) % 9 != 0 && a/9 != b/9) {
                        binary_constraints[a][bin_constr_counts[a]++] = DIFF_CONSTRAINT{a, b};
                        binary_constraints[b][bin_constr_counts[b]++] = DIFF_CONSTRAINT{a, b};
                    }
                }
            }
        }
    }

    // sort each bin
    for (auto &arr : binary_constraints) {
        std::sort(std::begin(arr), std::end(arr));
    }

    return binary_constraints;
}


// opens the input file and updates domains according to the unary constraints
bool apply_unary_constraints(const std::string& input_filename, CSP &csp) {

    char ch;
    unsigned short i = 0;
    std::ifstream file_in(input_filename, std::ifstream::in);
    while (file_in >> std::noskipws >> ch) {
        if ('1' <= ch && ch <= '9') {
            csp[i++] = 1 << (ch - '0' - 1);
        }
        else if (ch != '\n') {
            i++;
        }
    }

    // return true iff we did not read exactly 81 values from file
    bool bad_file = ( i != csp.size() );
    return bad_file;
}


// sums the size of every domain
// (this should decrease every time a domain is narrowed)
// returns 0 iff there is an empty domain
// returns 81 iff all 81 variables have been assigned
unsigned short domain_size_sum(std::array<unsigned short, 81> &csp) {
    unsigned short sum = 0U;
    // loop over every variable in the CSP
    for (const auto &domain : csp) {
        // number of set bits in a domain is the number of possible values
        sum += std::popcount(domain);
        if (domain == 0) {
            // empty domain ==> no solution
            return 0;
        }
    }
    return sum;
}


// helper method for AC-3
// removes elements from dom(var1) which are inconsistent with dom(var2)
bool revise(CSP &csp, unsigned short var1, unsigned short var2) {
    auto v1_domain_size_old = std::popcount(csp[var1]);
    // if |dom(var2)| == 1, then make sure that number isn't in dom(var1)
    if (std::has_single_bit(csp[var2])) {
        csp[var1] &= ~csp[var2];
    }
    auto v1_domain_size_new = std::popcount(csp[var1]);
    return v1_domain_size_old != v1_domain_size_new;
}


//  AC-3 implementation
unsigned short make_arc_consistent(CSP &csp, const CONSTRAINTS_MATRIX &bin_constraints) {

    // create a queue
    std::set<DIFF_CONSTRAINT> queue;

    // put every binary constraint in the queue
    for (const auto &arr : bin_constraints) {
        queue.insert(std::cbegin(arr), std::cend(arr));
    }

    // for keeping track of the size of the queue (as is apparently required)
    std::ofstream queue_size_log_file("queue_size.txt", std::ifstream::trunc);
    queue_size_log_file << queue.size() << std::endl;

    do {
        // pop an arbitrary arc from the queue
        auto arc = queue.begin();
        auto var1 = arc -> first;
        auto var2 = arc -> second;
        queue.erase(arc);

        if (revise(csp, var1, var2)) {
            // dom(var1) was revised; put adjacent arcs in the queue
            queue.insert(std::cbegin(bin_constraints[var1]), std::cend(bin_constraints[var1]));
            if ( csp[var1] == 0 ) {
                return 0; // nothing in revised domain ==> no solution
            }
        }
        else if (revise(csp, var2, var1)) {
            // dom(var2) was revised; put adjacent arcs in the queue
            queue.insert(std::cbegin(bin_constraints[var2]), std::cend(bin_constraints[var2]));
            if ( csp[var1] == 0 ) {
                return 0; // nothing in revised domain ==> no solution
            }
        }

        // log the size of the queue (so we can generate a chart later, if need be)
        queue_size_log_file << queue.size() << std::endl;

    } while (queue.size() != 0);

    // close the file where we logged the size of the queue
    queue_size_log_file.close();

    return domain_size_sum(csp);
}


// check if assigning `assignment` to variable `var` violates a constraint
bool feasible_assignment(
                         CSP &csp,
                         const CONSTRAINTS_MATRIX &bin_constraints,
                         unsigned short var,
                         unsigned short assignment) {
    for (const auto &constraint : bin_constraints[var]) {
        if (csp[constraint.second] == assignment) {
            // same assignment as another variable in same row, column, or sub-grid
            return false;
        }
    }
    return true;
}


// only to be used after making the CSP arc-consistent
unsigned short solve_with_backtracking(CSP &csp, const CONSTRAINTS_MATRIX &bin_constraints) {
    // loop through every variable
    for (unsigned short i = 0; i < 81; i++) {
        // if the variable is unassigned (i.e., its domain has >1 value)
        if ( !std::has_single_bit(csp[i]) ) {
            unsigned short full_domain = csp[i];
            unsigned short temp_domain = csp[i];
            unsigned short offset = 0U;
            do {
                // try the next value in the domain
                offset += std::countr_zero(temp_domain) + 1;
                temp_domain >>= std::countr_zero(temp_domain) + 1;
                // only use this value if it is locally consistent
                if ( feasible_assignment(csp, bin_constraints, i, 1 << (offset - 1)) ) {
                    // try to solve the puzzle with this value
                    csp[i] = 1 << (offset - 1);
                    // recursively backtrack
                    if ( solve_with_backtracking(csp, bin_constraints) == 81) {
                        // base case: sub-problem is solved
                        return 81;
                    }
                    // no solution was found with this assignment
                    // undo this assignment
                    csp[i] = full_domain;
                }
            } while ( temp_domain != 0 );
            // exhausted every possible assignment for this variable
            // (in combination with all other unassigned variables)
            return 0;
        }
    }
    // found a solution
    return domain_size_sum(csp);
}


void write_solution_to_file(const std::string output_filename, const CSP &csp, std::string m) {
    // text file for the solved puzzle
    std::ofstream output_file(output_filename, std::ifstream::trunc);
    
    // keep track of which column is printed (so newlines can be added)
    unsigned short i = 0;
    for (const auto domain : csp) {
        if ( std::has_single_bit(domain) ) {
            // singleton domain ==> print the value
            output_file << std::countr_zero(domain) + 1;
        }
        else {
            // unsolved domain
            output_file << ' ';
        }
        // add a newline character after every 9 characters
        if (++i % 9 == 0) {
            output_file << std::endl;
        }
    }
    // write message `m` at the bottom line of the file
    output_file << m << std::endl;
}


// Driver code
int main(int argc, char *argv[]) {

    std::string input_filename{"puzzle_in.txt"};
    std::string output_filename{"puzzle_out.txt"};
    if (argc >= 2) input_filename = argv[1];
    if (argc >= 3) output_filename = argv[2];


    auto binary_constraints = generate_binary_constraints();

    // create 9x9 array of domains
    // each domain is a (short) integer
    // 1s values in the domain, 0s denote values not in the domain
    // for example, d & 1<<(5-1) iff 5 is in domain d
    CSP csp;
    std::fill(std::begin(csp), std::end(csp), 0b0000000111111111U);

    // reads the input file and updates domains appropriately
    bool file_failed = apply_unary_constraints(input_filename, csp);
    if (file_failed) {
        // terminate if the file could not run properly
        std::cerr << "Error: could not open file \"" << input_filename << "\""
            << " or that file has improper formatting" << std::endl;
        return 1;
    }

    // apply AC-3
    std::string message;
    auto dom_size = make_arc_consistent(csp, binary_constraints);
    if (dom_size == 0) {
        message.assign("This puzzle is unsolveable");
    }
    else if (dom_size == 1) {
        message.assign("Solved using AC-3 only");
    }
    else {
        // rearrange constraints to act like key->value pairs
        // rather than simply being in ascending order
        for (unsigned short i = 0; i < 81; i++) {
            for (auto &constraint : binary_constraints[i]) {
                if (constraint.first != i) {
                    std::swap(constraint.first, constraint.second);
                }
            }
        }

        // use backtracking to find a solution
        auto soln = solve_with_backtracking(csp, binary_constraints);
        if (soln == 0) {
            message.assign("This puzzle is unsolveable");
        }
        else if (soln == 81) {
            message.assign("Solved using AC-3 and backtracking");
        }
        else {
            // something in the code went wrong
            // unlclear whether or not puzzle is solveable
            message.assign("Error: result is indeterminate");
        }
    }

    write_solution_to_file(output_filename, csp, message);

    std::cout << "Terminated Successfully" << std::endl;
    return 0;
}
