USING_NAMESPACE_QPOASES

static const double INF = 1e+29;

/** Active-set QP algorithm for solving QPs
 *
 * \param H the quadratic term
 * \param c the linear term
 * \param lb the lower bound
 * \param ub the lower bound
 * \param M the linear inequality constraints (M*z >= q)
 * \param q the linear inequality bound (M*z >= q)
 * \param A the linear equality constraint (A*z = b)
 * \param b the linear equality constraint (A*z = b)
 * \param z a vector "close" to the solution on input (optional); contains the solution on output
 */
template <class Mat1, class Vec1, class Vec2, class Vec3, class Mat2, class Vec4, class Mat3, class Vec5, class Vec6>
bool QPOASES::qp_activeset(const Mat1& H, const Vec1& c, const Vec2& lb, const Vec3& ub, const Mat2& M, const Vec4& q, const Mat3& A, const Vec5& b, Vec6& z)
{
   // set the number of variables
   int n = c.rows();

   // resize z, if necessary
   z.resize((unsigned) n);

   // check the bounds
   assert(lb.size() == ub.size());
   assert(lb.size() == (unsigned) n);

   // check the constraint matrix
   assert(M.columns() == (unsigned) n);
   assert(A.columns() == (unsigned) n);

   // check the lower bounds and upper bounds
   assert(lb.size() == ub.size());
   assert(lb.size() == (unsigned) n);

   // set the number of constraints
   int m = M.rows() + A.rows();

   // Setup the problem
   QProblem problem(n, m);

   // configure the problem
   Options opts;
   opts.printLevel = PL_NONE;
   problem.setOptions(opts);

   // setup the constraint matrix
   Ravelin::MatrixNd _X;
   _X.resize(z.rows(), m);
   _X.set_sub_mat(0, 0, M, Ravelin::eTranspose);
   _X.set_sub_mat(0, M.rows(), A, Ravelin::eTranspose);

   // set the maximum number of working set recalculations
   int nSWR = 100000000;

   // setup lower bounds and upper bounds
   Ravelin::VectorNd _lb, _ub;
   _lb.set_zero(lb.rows());
   _ub.set_zero(ub.rows());
   _lb.set_sub_vec(0, lb);
   _ub.set_sub_vec(0, ub);

   // bound lower/upper bound constraints properly
   for (unsigned i = 0; i< (unsigned) n; i++)
   {
      if (_ub[i] > INF)
         _ub[i] = INF;
      if (_lb[i] < -INF)
         _lb[i] = -INF;
   }

   // setup equality and inequality constraints
   Ravelin::VectorNd _lbA, _ubA;
   _lbA.set_zero(_X.columns());
   _ubA.set_zero(_X.columns());

   // set the inequality constraint into the lower bound vector
   _lbA.set_sub_vec(0, q);

   // set the equality constraint into the lower bound vector
   _lbA.set_sub_vec((int) (q.rows()), b);

   // set the inequality constraint to INF in the upper bound vector
   std::fill_n(_ubA.begin(), q.rows(), INF);
   _ubA.set_sub_vec((int) q.rows(), b);

   returnValue result;
   try {
      result = problem.init(
         H.data(),
         c.data(),
         _X.data(),
         _lb.data(),
         _ub.data(),
         _lbA.data(),
         _ubA.data(),
         nSWR,
         NULL /* Maximum solution time */
      );
   }
   catch (std::runtime_error e)
   {
      return false;
   }

   // look whether failure is indicated
   if (result != SUCCESSFUL_RETURN) {
      std::cerr << "Failed to solve QP: " << result << std::endl;
      return false;
   }

   // extract the result
   result = problem.getPrimalSolution(z.data());

   // look whether failure is indicated
   if (result != SUCCESSFUL_RETURN) {
      std::cerr << "Failed to fetch primal solution: " << result << std::endl;
      return false;
   }

   // verify feasibility (M*z >= q)
   Ravelin::VectorNd _r;
   M.mult(z, _r) -= q;
   if (std::find_if(_r.begin(), _r.end(), std::bind2nd(std::less<double>(), -NEAR_ZERO)) != _r.end())
      return false;

   // verify feasibility (A*z = b)
   A.mult(z, _r) -= b;
   for (Ravelin::RowIteratord_const i = _r.row_iterator_begin(); i != _r.row_iterator_end(); i++)
      if (std::fabs(*i) > NEAR_ZERO)
         return false;

   // verify z >= lb and z <= ub
   for (unsigned i=0; i< n; i++)
      if ((z[i] < lb[i] && !rel_equal(z[i], lb[i])) ||
          (z[i] > ub[i] && !rel_equal(z[i], ub[i])))
         return false;

   // all checks passed
   return true;
}
