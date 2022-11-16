#include <algorithm>
#include <array>
#include <bit> // requires C++ 20
#include <fstream>
#include <iostream>
#include <string>
#include <set>

typedef unsigned short int DOMAIN;
typedef std::array<DOMAIN, 81> CSP;
typedef unsigned short int VARIABLE;
typedef std::pair<VARIABLE, VARIABLE> DIFF_CONSTRAINT;
typedef std::array<std::array<DIFF_CONSTRAINT, 20>, 81> CONSTRAINTS_MATRIX;


// generate a 2d array of all binary constraints
CONSTRAINTS_MATRIX generate_binary_constraints() {
    // binary_constraints[i] has every constraint on variable i
    CONSTRAINTS_MATRIX binary_constraints;
    
    // keep track of how many constraints are in each bin (i.e., the end of each bin)
    std::array<unsigned short, 81> bin_constr_counts;
    // start counting from 0
    std::fill(std::begin(bin_constr_counts), std::end(bin_constr_counts), 0U);

    // constraints for values in the same row
    for (unsigned short r = 0; r < 81; r += 9) {
        // r is the row
        for (VARIABLE c1 = r; c1 != r + 8; c1++) {
            for (VARIABLE c2 = c1 + 1; c2 != r + 9; c2++) {
                // c1 and c2 are columns in row r
                binary_constraints[c1][bin_constr_counts[c1]++] = DIFF_CONSTRAINT{c1, c2};
                binary_constraints[c2][bin_constr_counts[c2]++] = DIFF_CONSTRAINT{c1, c2};
            }
        }
    }

    // constraints for values in the same column
    for (unsigned short c = 0; c != 9; c++) {
        // c is the column
        for (VARIABLE r1 = c; r1 < 81; r1 += 9) {
            for (VARIABLE r2 = r1 + 9; r2 < 81; r2 += 9) {
                // r1 and r2 are row in column c
                binary_constraints[r1][bin_constr_counts[r1]++] = DIFF_CONSTRAINT{r1, r2};
                binary_constraints[r2][bin_constr_counts[r2]++] = DIFF_CONSTRAINT{r1, r2};
            }
        }
    }

    // constraints for values in the same sub-grid
    for (unsigned short br = 0; br < 81; br += 27) {
        for (auto bc = br; bc < br + 9; bc += 3) {
            // bc is the top-left corner of a 3x3 sub-grid
            // bc + (x/3)*9 + x%3 gives each value in the sub-grid
            // where 0 <= x < 9
            for (unsigned short i = 0; i != 8; i++) {
                VARIABLE a = bc + (i/3) * 9 + i%3;
                for (auto j = i + 1; j != 9; j++) {
                    VARIABLE b = bc + (j/3) * 9 + j%3;
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
bool apply_unary_constraints(const std::string &input_filename, CSP &csp) {

    char ch;
    unsigned short i = 0;
    // open the requested file
    std::ifstream file_in(input_filename, std::ifstream::in);

    // read one character at a time
    while (file_in >> std::noskipws >> ch) {
        if ('1' <= ch && ch <= '9') {
            csp[i++] = 1 << (ch - '0');
        }
        else if (ch == '\n') {
            // ensure rows are aligned properly
            // (if a file does not have exactly 9 characters per row)
            // this fails only if a line is completely empty
            i = ((i - 1) / 9 + 1) * 9;
        }
        else {
            // all characters besides digits are treated as unknown
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
unsigned short domain_size_sum(CSP &csp) {
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
bool revise(CSP &csp, VARIABLE var1, VARIABLE var2) {
    auto v1_domain_size_old = std::popcount(csp[var1]);
    // if |dom(var2)| == 1, then make sure that number isn't in dom(var1)
    if (std::has_single_bit(csp[var2])) {
        csp[var1] &= ~csp[var2];
    }
    return v1_domain_size_old != std::popcount(csp[var1]);
}


//  AC-3 implementation
void make_arc_consistent(CSP &csp, const CONSTRAINTS_MATRIX &bin_constraints) {

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
        VARIABLE var1 = arc -> first;
        VARIABLE var2 = arc -> second;
        queue.erase(arc);

        if ( revise(csp, var1, var2) ) {
            // dom(var1) was revised; put adjacent arcs in the queue
            queue.insert(std::cbegin(bin_constraints[var1]), std::cend(bin_constraints[var1]));
            if ( csp[var1] == 0 ) {
                queue.clear(); // revised domain is empty ==> no solution ==> stop AC-3
            }
        }
        else if ( revise(csp, var2, var1) ) {
            // dom(var2) was revised; put adjacent arcs in the queue
            queue.insert(std::cbegin(bin_constraints[var2]), std::cend(bin_constraints[var2]));
            if ( csp[var1] == 0 ) {
                queue.clear(); // revised domain is empty ==> no solution  ==> stop AC-3
            }
        }

        // log the size of the queue (so we can generate a chart later, if need be)
        queue_size_log_file << queue.size() << std::endl;

    } while ( queue.size() != 0 );

    // close the file where we logged the size of the queue
    queue_size_log_file.close();
}


// check if assigning `assignment` to variable `var` violates a constraint
bool feasible_assignment(
                         CSP &csp,
                         const std::array<DIFF_CONSTRAINT, 20> &bin_constraints,
                         DOMAIN assignment) {
    for (const auto &constraint : bin_constraints) {
        if (csp[constraint.second] == assignment) {
            // same assignment as another variable in same row, column, or sub-grid
            return false;
        }
    }
    // none of the relevant constraints were violated
    return true;
}


// only to be used after making the CSP arc-consistent
unsigned short solve_with_backtracking(
                                       CSP &csp,
                                       const CONSTRAINTS_MATRIX &bin_constraints,
                                       VARIABLE i) {
    // loop through every variable
    while ( i != 81 && std::has_single_bit(csp[i]) ) {
        // VARIABLE i has already been narrowed to a singleton domain
        // move on to the next VARIABLE
        i++;
    }

    if (i == 81) {
        // all 81 variables have feasible assignments
        // ==> current csp is a solution
        return 81;
    }

    DOMAIN full_domain = csp[i];
    DOMAIN temp_domain = full_domain;
    unsigned short offset = -1;
    do {
        // try the next value in the domain
        offset += std::countr_zero(temp_domain) + 1;
        temp_domain >>= std::countr_zero(temp_domain) + 1;
        // only use this value if it is locally consistent
        if ( feasible_assignment(csp, bin_constraints[i], 1 << offset) ) {
            // try to solve the puzzle with this value
            csp[i] = 1 << offset;
            // recursively backtrack
            if ( solve_with_backtracking(csp, bin_constraints, i + 1) == 81 ) {
                // base case: sub-problem is solved
                return 81;
            }
            // no solution was found with this assignment
            // undo the assignment
            csp[i] = full_domain;
        }
    } while ( temp_domain != 0 );
    // exhausted every possible assignment for VARIABLE i
    // in combination with all other unassigned variables
    return 0;

}


void write_solution_to_file(const std::string filename_out, const CSP &csp, std::string msg) {
    // text file for the solved puzzle
    std::ofstream output_file(filename_out, std::ifstream::trunc);
    
    // keep track of which column is printed (so newlines can be added)
    VARIABLE i = 0;
    for (const auto &domain : csp) {
        if ( std::has_single_bit(domain) ) {
            // singleton domain ==> print the value
            output_file << std::countr_zero(domain);
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
    // write message `msg` to the last line of the file
    output_file << msg << std::endl;
}


void write_domains_to_file(const std::string filename_out, const CSP &csp) {
    // text file for the solved puzzle
    std::ofstream output_file(filename_out, std::ifstream::trunc);
    
    // keep track of which column is printed (so newlines can be added)
    VARIABLE i = 0;
    for (auto domain : csp) {
        for (unsigned short i = 1; i != 10; i++) {
            domain >>= 1;
            if ( (domain & 1) != 0 ) {
                output_file << i;
            }
            else {
                output_file << ' ';
            }
        }
        
        if (++i % 9 == 0) {
            // newline character between rows
            output_file << std::endl;
            if (i % 27 == 0 && i != 81) {
                // draw horizontal lines to split up 3x3 sug-grids
                for (unsigned short i = 0; i != 8; i++) {
                    output_file << std::string(9, '-') << '+';
                }
                output_file << std::string(9, '-') << std::endl;
            }
        }
        else {
            output_file << '|'; // seperator between each domain
        }
    }
}


// Driver code
int main(int argc, char *argv[]) {

    std::string input_filename{"puzzle_in.txt"};
    std::string output_filename{"puzzle_out.txt"};
    if (argc >= 2) input_filename = argv[1];
    if (argc >= 3) output_filename = argv[2];


    CONSTRAINTS_MATRIX binary_constraints = generate_binary_constraints();

    // create 9x9 array of domains
    // each domain is a (short) integer
    // 1s values in the domain, 0s denote values not in the domain
    // for example, d & 1<<5 iff 5 is in domain d
    CSP csp;
    std::fill(std::begin(csp), std::end(csp), 0b1111111110U);

    // reads the input file and updates domains appropriately
    bool file_failed = apply_unary_constraints(input_filename, csp);
    if (file_failed) {
        // terminate if the file could not run properly
        std::cerr << "Error: could not open file \"" << input_filename << "\""
            << " or that file has improper formatting" << std::endl;
        return 1;
    }

    // tells the user how the puzzle was solved
    std::string message;

    if (domain_size_sum(csp) == 81) {
        message.assign("Puzzle is already solved");
    } else {

        // apply AC-3
        make_arc_consistent(csp, binary_constraints);

        // writes the domains to a text file
        write_domains_to_file("arc-consistent-csp.txt", csp);

        // check domain sizes
        auto dom_size = domain_size_sum(csp);
        if (dom_size == 0) {
            // at least one empty domain
            message.assign("This puzzle is unsolveable");
        }
        else if (dom_size == 81) {
            // all 81 domains are singletons
            message.assign("Solved using AC-3 only");
        }
        else {
            // rearrange constraints to act like key->value pairs
            // rather than simply being in ascending order
            for (VARIABLE var = 0; var != 81; var++) {
                for (auto &constraint : binary_constraints[var]) {
                    if (constraint.first != var) {
                        std::swap(constraint.first, constraint.second);
                    }
                }
            }

            // use backtracking to find a solution
            auto size = solve_with_backtracking(csp, binary_constraints, 0);
            if (size == 0) {
                message.assign("This puzzle is unsolveable");
            }
            else if (size == 81) {
                message.assign("Solved using AC-3 and backtracking");
            }
            else {
                // something in the code went wrong
                // unlclear whether or not puzzle is solveable
                message.assign("Error: result is unknown...");
            }
        }
    }

    // the solution and message are written to a text file (not stdout)
    write_solution_to_file(output_filename, csp, message);

    return 0;
}
