/***********************************************************************************
Copyright (c) 2017, Agile & Dexterous Robotics Lab, ETH ZURICH. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of ETH ZURICH nor the names of its contributors may be used
      to endorse or promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL ETH ZURICH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#include <ct/optcon/optcon.h>

namespace ct{
namespace optcon{


IpoptSolver::IpoptSolver(std::shared_ptr<Nlp> nlp, const NlpSolverSettings& settings) :
BASE(nlp, settings),
settings_(BASE::settings_.ipoptSettings_)
{
	//Constructor arguments
	//Argument 1: create console output
	//Argument 2: create empty
	ipoptApp_ = std::shared_ptr<Ipopt::IpoptApplication> (new Ipopt::IpoptApplication(true,false));
}


IpoptSolver::~IpoptSolver()
{
	// Needed to destruct all the IPOPT memory
	Ipopt::Referencer* t=NULL;
	this->ReleaseRef(t);
	delete t;
}


void IpoptSolver::configureDerived(const NlpSolverSettings& settings)
{
	std::cout << "calling Ipopt configure derived" << std::endl;
	settings_ = settings.ipoptSettings_;
	setSolverOptions();
	isInitialized_ = true;
}


void IpoptSolver::setSolverOptions()
{
	ipoptApp_->Options()->SetNumericValue("tol", settings_.tol_);
	ipoptApp_->Options()->SetNumericValue("constr_viol_tol", settings_.constr_viol_tol_);
	ipoptApp_->Options()->SetIntegerValue("max_iter", settings_.max_iter_);
	// ipoptApp_->Options()->SetNumericValue("resto.tol", settings_.restoTol_);
	// ipoptApp_->Options()->SetNumericValue("acceptable_tol", settings_.acceptableTol_);
	// ipoptApp_->Options()->SetNumericValue("resto.acceptable_tol", settings_.restoAcceptableTol_);
	ipoptApp_->Options()->SetStringValueIfUnset("linear_scaling_on_demand", settings_.linear_scaling_on_demand_);
	ipoptApp_->Options()->SetStringValueIfUnset("hessian_approximation", settings_.hessian_approximation_);
	// ipoptApp_->Options()->SetStringValueIfUnset("nlp_scaling_method", settings_.nlp_scaling_method_);
	ipoptApp_->Options()->SetIntegerValue("print_level", settings_.printLevel_); //working now
	ipoptApp_->Options()->SetStringValueIfUnset("print_user_options", settings_.print_user_options_);
	// ipoptApp_->Options()->SetIntegerValue("print_frequency_iter", settings_.print_frequency_iter_);
	ipoptApp_->Options()->SetStringValueIfUnset("derivative_test", settings_.derivativeTest_);
	ipoptApp_->Options()->SetIntegerValue("print_level", settings_.printLevel_);
	ipoptApp_->Options()->SetNumericValue("derivative_test_tol", settings_.derivativeTestTol_);
	ipoptApp_->Options()->SetNumericValue("derivative_test_perturbation", settings_.derivativeTestPerturbation_);
	ipoptApp_->Options()->SetNumericValue("point_perturbation_radius", settings_.point_perturbation_radius_);
	ipoptApp_->Options()->SetStringValueIfUnset("linear_system_scaling", settings_.linearSystemScaling_);
	ipoptApp_->Options()->SetStringValueIfUnset("linear_solver", settings_.linear_solver_);
}


bool IpoptSolver::solve()
{
	status_ = ipoptApp_->Initialize();
	if (status_ == Ipopt::Solve_Succeeded)
		std::cout << std::endl << std::endl	<< "*** Initialized successfully -- starting NLP." << std::endl;
	else
		throw(std::runtime_error("NLP initialization failed"));

	// Ask Ipopt to solve the problem
	status_ = ipoptApp_->OptimizeTNLP(this);

	if (status_ == Ipopt::Solve_Succeeded || status_ == Ipopt::Solved_To_Acceptable_Level) {
		// Retrieve some statistics about the solve
		Ipopt::Index iter_count = ipoptApp_->Statistics()->IterationCount();
		std::cout << std::endl << std::endl << "*** The problem solved in " << iter_count << " iterations!" << std::endl;

		Number final_obj = ipoptApp_->Statistics()->FinalObjective();
		std::cout << std::endl << std::endl << "*** The final value of the objective function is " << final_obj << '.' << std::endl;
		return true;
	}
	else{
		std::cout << " ipopt return value: " << status_ << std::endl;
		return false;
	}
}


void IpoptSolver::prepareWarmStart(size_t maxIterations)
{
	ipoptApp_->Options()->SetStringValue("warm_start_init_point", "yes");
	ipoptApp_->Options()->SetNumericValue("warm_start_bound_push", 1e-9);
	ipoptApp_->Options()->SetNumericValue("warm_start_bound_frac", 1e-9);
	ipoptApp_->Options()->SetNumericValue("warm_start_slack_bound_frac", 1e-9);
	ipoptApp_->Options()->SetNumericValue("warm_start_slack_bound_push", 1e-9);
	ipoptApp_->Options()->SetNumericValue("warm_start_mult_bound_push", 1e-9);
	ipoptApp_->Options()->SetIntegerValue("max_iter", (int)maxIterations);
	ipoptApp_->Options()->SetStringValue("derivative_test", "none");
}


bool IpoptSolver::get_nlp_info(Ipopt::Index& n, Ipopt::Index& m, Ipopt::Index& nnz_jac_g,
		Ipopt::Index& nnz_h_lag, IndexStyleEnum& index_style)
{
	n = nlp_->getVarCount();
	assert(n == n);

	m = nlp_->getConstraintsCount();
	assert(m == m);

	nnz_jac_g = nlp_->getNonZeroJacobianCount();
	assert(nnz_jac_g==nnz_jac_g);

	nnz_h_lag = nlp_->getNonZeroHessianCount();

	index_style = Ipopt::TNLP::C_STYLE;

#ifdef DEBUG_PRINT
	std::cout << "... number of decision variables = " << n << std::endl;
	std::cout << "... number of constraints = " << m << std::endl;
	std::cout << "... nonzeros in jacobian = " << nnz_jac_g << std::endl;
#endif

	return true;
}


bool IpoptSolver::get_bounds_info(Ipopt::Index n, Number* x_l, Number* x_u,
		Ipopt::Index m, Number* g_l, Number* g_u)
{
#ifdef DEBUG_PRINT
	std::cout << "... entering get_bounds_info()" << std::endl;
#endif //DEBUG_PRINT
	MapVecXd x_lVec(x_l, n);
	MapVecXd x_uVec(x_u, n);
	MapVecXd g_lVec(g_l, m);
	MapVecXd g_uVec(g_u, m);
	nlp_->getVariableBounds(x_lVec, x_uVec, n);
	// bounds on optimization vector
	// x_l <= x <= x_u
	assert(x_l == x_l);
	assert(x_u == x_u);
	assert(n == n);

	// constraints bounds (e.g. for equality constraints = 0)
	nlp_->getConstraintBounds(g_lVec, g_uVec, m);
	assert(g_l == g_l);
	assert(g_u == g_u);

#ifdef DEBUG_PRINT
	std::cout << "... Leaving get_bounds_info()" << std::endl;
#endif //DEBUG_PRINT

	return true;
}


bool IpoptSolver::get_starting_point(Ipopt::Index n, bool init_x, Number* x,
		bool init_z, Number* z_L, Number* z_U,
		Ipopt::Index m, bool init_lambda,
		Number* lambda)
{
#ifdef DEBUG_PRINT
	std::cout << "... entering get_starting_point()" << std::endl;
#endif //DEBUG_PRINT
       //
	if(init_x)
	{
		MapVecXd xVec(x, n);
		nlp_->getOptimizationVars(n, xVec);
	}

	if(init_z)
	{
		MapVecXd z_lVec(z_L, n);
		MapVecXd z_uVec(z_U, n);
		nlp_->getBoundMultipliers(n, z_lVec, z_uVec);
	}

	if(init_lambda)
	{
		MapVecXd lambdaVec(lambda, m);
		nlp_->getLambdaVars(m, lambdaVec);
	}


#ifdef DEBUG_PRINT
	std::cout << "... entering get_starting_point()" << std::endl;
#endif //DEBUG_PRINT

	return true;
}


bool IpoptSolver::eval_f(Ipopt::Index n, const Number* x, bool new_x, Number& obj_value)
{
#ifdef DEBUG_PRINT
	std::cout << "... entering eval_f()" << std::endl;
#endif //DEBUG_PRINT
	MapConstVecXd xVec(x, n);
	nlp_->extractOptimizationVars(xVec, new_x);
	obj_value = nlp_->evaluateCostFun();
	assert(obj_value == obj_value);

#ifdef DEBUG_PRINT
	std::cout << "... leaving eval_f()" << std::endl;
#endif //DEBUG_PRINT
	return true;
}


bool IpoptSolver::eval_grad_f(Ipopt::Index n, const Number* x, bool new_x, Number* grad_f)
{
#ifdef DEBUG_PRINT
	std::cout << "... entering eval_grad_f()" << std::endl;
#endif //DEBUG_PRINT
	MapVecXd grad_fVec(grad_f, n);
	MapConstVecXd xVec(x, n);
	nlp_->extractOptimizationVars(xVec, new_x);
	nlp_->evaluateCostGradient(n, grad_fVec);

#ifdef DEBUG_PRINT
	std::cout << "... leaving eval_grad_f()" << std::endl;
#endif //DEBUG_PRINT
	return true;
}


bool IpoptSolver::eval_g(Ipopt::Index n, const Number* x, bool new_x, Ipopt::Index m, Number* g)
{
#ifdef DEBUG_PRINT
	std::cout << "... entering eval_g()" << std::endl;
#endif //DEBUG_PRINT
	assert(m == nlp_->getConstraintsCount());
	MapConstVecXd xVec(x, n);
	nlp_->extractOptimizationVars(xVec, new_x);
	MapVecXd gVec(g, m);
	nlp_->evaluateConstraints(gVec);


#ifdef DEBUG_PRINT
	std::cout << "gVec: " << gVec.transpose() << std::endl;
	std::cout << "... leaving eval_g()" << std::endl;
#endif //DEBUG_PRINT

	return true;
}


bool IpoptSolver::eval_jac_g(Ipopt::Index n, const Number* x, bool new_x,
		Ipopt::Index m, Ipopt::Index nele_jac, Ipopt::Index* iRow, Ipopt::Index *jCol,
		Number* values)
{
	if (values == NULL)
	{
#ifdef DEBUG_PRINT
		std::cout << "... entering eval_jac_g, values == NULL" << std::endl;
#endif //DEBUG_PRINT
		// set indices of nonzero elements of the jacobian
		Eigen::Map<Eigen::VectorXi> iRowVec(iRow, nele_jac);
		Eigen::Map<Eigen::VectorXi> jColVec(jCol, nele_jac);
		nlp_->getSparsityPatternJacobian(nele_jac, iRowVec, jColVec);


#ifdef DEBUG_PRINT
		std::cout << "... leaving eval_jac_g, values == NULL" << std::endl;
#endif //DEBUG_PRINT
	}
	else
	{
#ifdef DEBUG_PRINT
		std::cout << "... entering eval_jac_g, values != NULL" << std::endl;
#endif //DEBUG_PRINT
		MapVecXd valVec(values, nele_jac);
		MapConstVecXd xVec(x, n);
		nlp_->extractOptimizationVars(xVec, new_x);
		nlp_->evaluateConstraintJacobian(nele_jac, valVec);


#ifdef DEBUG_PRINT
		std::cout << "... leaving eval_jac_g, values != NULL" << std::endl;
#endif //DEBUG_PRINT
	}

	return true;
}


bool IpoptSolver::eval_h(Ipopt::Index n, const Number* x, bool new_x,
		Number obj_factor, Ipopt::Index m, const Number* lambda,
		bool new_lambda, Ipopt::Index nele_hess, Ipopt::Index* iRow,
		Ipopt::Index* jCol, Number* values)
{

#ifdef DEBUG_PRINT
	std::cout << "... entering eval_h()" << std::endl;
#endif
	if (values == NULL)
	{
		// return the structure. This is a symmetric matrix, fill the lower left
		// triangle only.
		Eigen::Map<Eigen::VectorXi> iRowVec(iRow, nele_hess);
		Eigen::Map<Eigen::VectorXi> jColVec(jCol, nele_hess);
		nlp_->getSparsityPatternHessian(nele_hess, iRowVec, jColVec);
	}
	else
	{
		// return the values. This is a symmetric matrix, fill the lower left
		// triangle only
		MapVecXd valVec(values, nele_hess);
		MapConstVecXd xVec(x, n);
		MapConstVecXd lambdaVec(lambda, m);
		nlp_->extractOptimizationVars(xVec, new_x);
		nlp_->evaluateHessian(nele_hess, valVec, obj_factor, lambdaVec);
	}

	// only needed if quasi-newton approximation is not used, hence set to -1 (not used)!
	// ATTENTION: for hard coding of the hessian, one only needs the lower left corner (since it is symmetric) - IPOPT knows that
	//nnz_h_lag = -1;

#ifdef DEBUG_PRINT
	std::cout << "... leaving eval_h()" << std::endl;
#endif

	return true;
}


void IpoptSolver::finalize_solution(Ipopt::SolverReturn status,
		Ipopt::Index n, const Number* x, const Number* z_L, const Number* z_U,
		Ipopt::Index m, const Number* g, const Number* lambda,
		Number obj_value,
		const Ipopt::IpoptData* ip_data,
		Ipopt::IpoptCalculatedQuantities* ip_cq)
{
#ifdef DEBUG_PRINT
	std::cout << "... entering finalize_solution() ..." << std::endl;
#endif
	MapConstVecXd xVec(x, n);
	MapConstVecXd zLVec(z_L, n);
	MapConstVecXd zUVec(z_U, n);
	MapConstVecXd lambdaVec(lambda, m);

	nlp_->extractIpoptSolution(xVec, zLVec, zUVec, lambdaVec);
}

} // namespace optcon
} // namespace ct
