
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <coin/ClpSimplex.hpp>
#include <coin/CoinPackedMatrix.hpp>
#include <coin/CoinPackedVector.hpp>
#include <cmath>

#include "src/model/panda.h"

template<GlobalSchema...>
class PANDASubproblemLinearProgram {
private:

    static constexpr std::size_t N = sizeof...(GlobalSchema);
    static constexpr std::size_t DIM = 1 << N;
    static std::size_t index_counter = 0;

    std::vector<attr_type<GlobalSchema...>> all_attrs;
    std::vector<Monotonicity<GlobalSchema...>> all_mons;
    std::vector<Submodularity<GlobalSchema...>> all_subs;
    std::unordered_map<attr_type<GlobalSchema...>> attr_indices;
    std::unordered_map<attr_type<Monotonicity<GlobalSchema...>> mon_indices;
    std::unordered_map<attr_type<Submodularity<GlobalSchema...>> sub_indices;

    
    void generate_all_attrs() {
        attr_type<GlobalSchema...> empty_attr;
        all_attrs.push_back(empty_attr);
        all_attrs[empty_attr] = index_counter;
        index_counter++;

        // iterate over all current attrs and re-add the same attr but with the i'th index toggled to 1
        for (std::size_t i = 0; i < N; i++) {            
            std::size_t curr_size = all_attrs.size();
            for (std::size_t j = 0; j < curr_size; j++) {
                attr_type<GlobalSchema...> toggled = all_attrs[j]
                toggled[i] = 1;
                all_attrs.push_back(toggled);
                attr_indices[empty_attr] = index_counter;
                index_counter++;
            }
        }
    }
    
    void generate_all_mons() {
        // iterate over all pairwise combinations of attrs and select all disjoint pairs
        for (std::size_t i = 0; i < DIM; i++) {
            attr_type<GlobalSchema...> attr_X = all_attrs[i];
            for (std::size_t j = 0; j < i; j++) {
                attr_type<GlobalSchema...> attr_Y = all_attrs[j];
                if (attr_X & attr_Y == NULL_ATTR<GlobalSchema...>) {
                    Monotonicity<GlobalSchema...> mon_Y_X = Monotonicity<GlobalSchema...>{
                        attr_Y,
                        attr_X
                    };
                    all_mons.push_back(mon_Y_X);
                    mon_indices[mon_Y_X] = index_counter;
                    index_counter++;
                }
            }
        }
    }
    
    void generate_all_subs() {
        // iterate over all 3-wise combinations of attrs and select all disjoint triples
        for (std::size_t i = 0; i < DIM; i++) {
            attr_type<GlobalSchema...> attr_X = all_attrs[i];
            for (std::size_t j = 0; j < i; j++) {
                attr_type<GlobalSchema...> attr_Y = all_attrs[j];
                for (std::size_t k = 0; k < j; k++) {
                    attr_type<GlobalSchema...> attr_Z = all_attrs[k];
                    if (attr_Z != NULL_ATTR<GlobalSchema...>
                            && attr_X & attr_Y & attr_Z == NULL_ATTR<GlobalSchema...>) {
                        Monotonicity<GlobalSchema...> sub_Y_Z_X = Submodularity<GlobalSchema...>{
                            attr_Y,
                            attr_Z,
                            attr_X
                        };
                        all_subs.push_back(sub_Y_Z_X);
                        sub_indices[sub_Y_Z_X] = index_counter;
                        index_counter++;
                    }
                }
            }
        }
    }

    std::tuple<CoinPackedVector, double, double> generate_shannon_constraint(std::unordered_map<attr_type<GlobalSchema...>, double> attr_coeffs) {
        CoinPackedVector constraint;
        for (const auto& [attrs, coeff] : attr_coeffs) {
            constraint.insert(attr_indices[attrs], attr_coeffs)
        }
    }

    std::vector<std::tuple<CoinPackedVector, double, double>> generate_mon_polymatroid_constraints() {
        std::vector<std::tuple<CoinPackedVector, double, double>> ret;
        for (const auto& mon : all_mons) {
            attr_type<GlobalSchema...> attrs_XY = mon.attrs_X ^ mon.attrs_Y;
            attr_type<GlobalSchema...> attrs_X = mon.attrs_X;

            CoinPackedVector definition_constraint;
            definition_constraint.insert(attr_indices[attrs_XY], 1.0);
            definition_constraint.insert(attr_indices[attrs_X], -1.0);
            definition_constraint.insert(mon_indices[mon], -1.0);
            ret.push_back(std::make_tuple(definition_constraint, 0.0, 0.0));

            CoinPackedVector polymatroid_constraint;
            polymatroid_constraint.insert(mon_indices[mon], 1.0);
            ret.push_back(std::make_tuple(polymatroid_constraint, 0.0, COIN_DBL_MAX));
        }
        return ret;
    }

    std::vector<std::tuple<CoinPackedVector, double, double>> generate_sub_polymatroid_constraints() {
        std::vector<std::tuple<CoinPackedVector, double, double>> ret;
        for (const auto& sub : all_subs) {
            attr_type<GlobalSchema...> attrs_XY = sub.attrs_X ^ sub.attrs_Y;
            attr_type<GlobalSchema...> attrs_XZ = sub.attrs_X ^ sub.attrs_Z;
            attr_type<GlobalSchema...> attrs_X = mon.attrs_X;
            attr_type<GlobalSchema...> attrs_XYZ = sub.attrs_X ^ sub.attrs_Y ^ sub.attrs_Z;

            CoinPackedVector definition_constraint;
            definition_constraint.insert(attr_indices[attrs_XY], 1.0);
            definition_constraint.insert(attr_indices[attrs_XZ], 1.0);
            definition_constraint.insert(attr_indices[attrs_X], -1.0);
            definition_constraint.insert(attr_indices[attrs_XYZ], -1.0);
            definition_constraint.insert(sub_indices[sub], -1.0);
            ret.push_back(std::make_tuple(definition_constraint, 0.0, COIN_DBL_MAX));

            CoinPackedVector polymatroid_constraint;
            polymatroid_constraint.insert(sub_indices[sub], 1.0);
            ret.push_back(std::make_tuple(polymatroid_constraint, 0.0, COIN_DBL_MAX));
        }
        return ret;
    }
    
public:
    PANDALinearProgram(int num_attrs) : num_attributes(num_attrs) {
        generate_all_attrs();
        generate_all_subs();
        generate_all_subs();
    }
    
    std::pair<std::map<Monotonicity, unsigned>, std::map<Submodularity, unsigned>> solvePANDA() {
        ClpSimplex model;
        
        std::size_t num_constraints = 1 + 2 * all_mons.size() + 2 * all_subs.size();
        std::size_t num_vars = all_attrs.size() + all_mons.size() + all_subs.size();

        // Set up the variables: m_d for monotonicities and s_u for submodularities
        model.resize(num_constraints, num_vars); // 1 constraint, numVars variables
        
        // All variables are free (can be positive or negative)
        for (std::size_t i = 0; i < num_vars; i++) {
            model.setColumnLower(i, -COIN_DBL_MAX);
            model.setColumnUpper(i, COIN_DBL_MAX);
        }







        
        // Set up the constraint: sum(a_X * h(X|0)) = sum(m_d * h(d)) + sum(s_u * h(u))
        CoinPackedVector row;
        
        // Calculate the right-hand side: sum(a_X * h(X|0))
        double rhs = 0.0;
        for (const auto& pair : a_coefficients) {
            rhs += pair.second * h_X_given_empty(pair.first);
        }
        
        // For the monotonicity coefficients in the constraint
        for (const auto& mono : monotonicities) {
            int col_idx = monotonicity_indices[mono];
            // h(d) for monotonicity is typically 1 in the standard formulation
            // This can be replaced with the actual h(d) calculation if needed
            row.insert(col_idx, 1.0);
        }
        
        // For the submodularity coefficients in the constraint
        for (const auto& sub : submodularities) {
            int col_idx = monotonicities.size() + submodularity_indices[sub];
            // h(u) for submodularity is typically 1 in the standard formulation
            // This can be replaced with the actual h(u) calculation if needed
            row.insert(col_idx, 1.0);
        }
        
        // Set the constraint
        model.setRowLower(0, rhs);
        model.setRowUpper(0, rhs);
        
        // Add the constraint to the model
        CoinPackedMatrix matrix;
        matrix.appendRow(row);
        model.replaceMatrix(&matrix);
        
        // Solve the LP
        model.initialSolve();
        
        // Check if we found a feasible solution
        if (model.isProvenOptimal() || model.isProvenPrimalInfeasible()) {
            std::cout << "LP solution status: " << model.statusString() << std::endl;
            
            // Extract the solutions
            const double* solution = model.primalColumnSolution();
            
            // Prepare the results
            std::map<Monotonicity, double> m_values;
            std::map<Submodularity, double> s_values;
            
            // Extract monotonicity coefficients
            for (const auto& mono : monotonicities) {
                int idx = monotonicity_indices[mono];
                m_values[mono] = solution[idx];
            }
            
            // Extract submodularity coefficients
            for (const auto& sub : submodularities) {
                int idx = monotonicities.size() + submodularity_indices[sub];
                s_values[sub] = solution[idx];
            }
            
            return {m_values, s_values};
        } else {
            std::cerr << "Failed to find a feasible solution." << std::endl;
            return {{}, {}};
        }
    }
    
    // Print information about the LP size
    void printInfo() {
        std::cout << "PANDA Linear Program Info:" << std::endl;
        std::cout << "Number of attributes: " << num_attributes << std::endl;
        std::cout << "Number of subsets: " << all_subsets.size() << std::endl;
        std::cout << "Number of monotonicity constraints: " << monotonicities.size() << std::endl;
        std::cout << "Number of submodularity constraints: " << submodularities.size() << std::endl;
        std::cout << "Total number of variables: " << monotonicities.size() + submodularities.size() << std::endl;
    }
};

// Example usage
int main() {
    // Create a PANDA LP with 3 attributes
    PANDALinearProgram panda(3);
    
    // Print information about the LP
    panda.printInfo();
    
    // Create some example a_X coefficients for all subsets
    std::map<AttributeSet, double> coeffs;
    
    // Set coefficients for all subsets of {0,1,2}
    coeffs[{}] = 0.0;
    coeffs[{0}] = 0.5;
    coeffs[{1}] = 0.3;
    coeffs[{2}] = 0.2;
    coeffs[{0, 1}] = 0.7;
    coeffs[{0, 2}] = 0.6;
    coeffs[{1, 2}] = 0.4;
    coeffs[{0, 1, 2}] = 0.9;
    
    panda.setCoefficients(coeffs);
    
    // Solve the LP
    auto [m_values, s_values] = panda.solvePANDA();
    
    // Print the results
    std::cout << "\nMonotonicity coefficients (m_d):" << std::endl;
    for (const auto& [mono, value] : m_values) {
        std::cout << mono.toString() << " = " << value << std::endl;
    }
    
    std::cout << "\nSubmodularity coefficients (s_u):" << std::endl;
    for (const auto& [sub, value] : s_values) {
        std::cout << sub.toString() << " = " << value << std::endl;
    }
    
    return 0;
}