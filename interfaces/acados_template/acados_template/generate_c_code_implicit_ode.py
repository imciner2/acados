#
# Copyright 2019 Gianluca Frison, Dimitris Kouzoupis, Robin Verschueren,
# Andrea Zanelli, Niels van Duijkeren, Jonathan Frey, Tommaso Sartor,
# Branimir Novoselnik, Rien Quirynen, Rezart Qelibari, Dang Doan,
# Jonas Koenemann, Yutao Chen, Tobias Schöls, Jonas Schlagenhauf, Moritz Diehl
#
# This file is part of acados.
#
# The 2-Clause BSD License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.;
#


from casadi import *
import os

def generate_c_code_implicit_ode( model, opts ):

    casadi_version = CasadiMeta.version()
    if  casadi_version not in ('3.4.5', '3.4.0'):
        # old casadi versions
        raise Exception('Please download and install Casadi 3.4.0 to ensure compatibility with acados. Version ' + casadi_version + ' currently in use.')

    casadi_opts = dict(mex=False, casadi_int='int', casadi_real='double')
    generate_hess = opts["generate_hess"]

    ## load model
    x = model.x
    xdot = model.xdot
    u = model.u
    z = model.z
    p = model.p
    f_impl = model.f_impl_expr
    model_name = model.name

    ## get model dimensions
    nx = x.size()[0]
    nu = u.size()[0]
    if type(z) is list:
        # check that z is empty
        if len(z) == 0:
            nz = 0
        else:
            raise Exception('z is a non-empty list. It should be either an empty list or an SX object.')
    else:
        nz = z.size()[0]

    if type(p) is list:
        # check that z is empty
        if len(p) == 0:
            np = 0
        else:
            raise Exception('p is a non-empty list. It should be either an empty list or an SX object.')
    else:
        np = p.size()[0]

    ## generate jacobians
    jac_x       = jacobian(f_impl, x)
    jac_xdot    = jacobian(f_impl, xdot)
    jac_u       = jacobian(f_impl, u)
    jac_z       = jacobian(f_impl, z)

    ## generate hessian
    x_xdot_z_u = vertcat(x, xdot, z, u)

    if type(x[0]) == casadi.SX:
        multiplier  = SX.sym('multiplier', nx + nz)
        multiply_mat  = SX.sym('multiply_mat', 2*nx+nz+nu, nx + nu)
        HESS = SX.zeros( x_xdot_z_u.size()[0], x_xdot_z_u.size()[0])
    elif type(x[0]) == casadi.MX:
        multiplier  = MX.sym('multiplier', nx + nz)
        multiply_mat  = MX.sym('multiply_mat', 2*nx+nz+nu, nx + nu)
        HESS = MX.zeros( x_xdot_z_u.size()[0], x_xdot_z_u,size()[0])

    for ii in range(f_impl.size()[0]):
        jac_x_xdot_z = jacobian(f_impl[ii], x_xdot_z_u)
        hess_x_xdot_z = jacobian( jac_x_xdot_z, x_xdot_z_u)
        HESS = HESS + multiplier[ii] * hess_x_xdot_z

    # HESS = HESS.simplify()
    HESS_multiplied = mtimes(mtimes(transpose(multiply_mat), HESS), multiply_mat)
    # HESS_multiplied = HESS_multiplied.simplify()

    ## Set up functions
    if np != 0:
        p = model.p
        fun_name = model_name + '_impl_dae_fun'
        impl_dae_fun = Function(fun_name, [x, xdot, u, z, p], [f_impl])

        fun_name = model_name + '_impl_dae_fun_jac_x_xdot_z'
        impl_dae_fun_jac_x_xdot_z = Function(fun_name, [x, xdot, u, z, p], [f_impl, jac_x, jac_xdot, jac_z])

        # fun_name = model_name + '_impl_dae_fun_jac_x_xdot_z'
        # impl_dae_fun_jac_x_xdot = Function(fun_name, [x, xdot, u, z, p], [f_impl, jac_x, jac_xdot, jac_z])

        # fun_name = model_name + '_impl_dae_jac_x_xdot_u'
        # impl_dae_jac_x_xdot_u = Function(fun_name, [x, xdot, u, z, p], [jac_x, jac_xdot, jac_u, jac_z])
        
        fun_name = model_name + '_impl_dae_fun_jac_x_xdot_u_z'
        impl_dae_fun_jac_x_xdot_u_z = Function(fun_name, [x, xdot, u, z, p], [f_impl, jac_x, jac_xdot, jac_u, jac_z])

        fun_name = model_name + '_impl_dae_fun_jac_x_xdot_u'
        impl_dae_fun_jac_x_xdot_u = Function(fun_name, [x, xdot, u, z, p], [f_impl, jac_x, jac_xdot, jac_u])

        fun_name = model_name + '_impl_dae_jac_x_xdot_u_z'
        impl_dae_jac_x_xdot_u_z = Function(fun_name, [x, xdot, u, z, p], [jac_x, jac_xdot, jac_u, jac_z])

        
        fun_name = model_name + '_impl_dae_hess'
        impl_dae_hess = Function(fun_name, [x, xdot, u, z, multiplier, multiply_mat, p], [HESS_multiplied])
    else:
        fun_name = model_name + '_impl_dae_fun'
        if nz > 0:
            impl_dae_fun = Function(fun_name, [x, xdot, u, z], [f_impl])
        else:
            impl_dae_fun = Function(fun_name, [x, xdot, u], [f_impl])
        
        fun_name = model_name + '_impl_dae_fun_jac_x_xdot_z'
        impl_dae_fun_jac_x_xdot_z = Function(fun_name, [x, xdot, u, z], [f_impl, jac_x, jac_xdot, jac_z])
        
        fun_name = model_name + '_impl_dae_fun_jac_x_xdot_u_z'
        impl_dae_fun_jac_x_xdot_u_z = Function(fun_name, [x, xdot, u, z], [f_impl, jac_x, jac_xdot, jac_u, jac_z])

        fun_name = model_name + '_impl_dae_jac_x_xdot_u_z'
        impl_dae_jac_x_xdot_u_z = Function(fun_name, [x, xdot, u, z], [jac_x, jac_xdot, jac_u, jac_z])
        
        fun_name = model_name + '_impl_dae_hess'
        impl_dae_hess = Function(fun_name, [x, xdot, u, z, multiplier, multiply_mat], [HESS_multiplied])

    # generate C code
    os.chdir('c_generated_code')
    model_dir = model_name + '_model'
    if not os.path.exists(model_dir):
        os.mkdir(model_dir)
    model_dir_location = './' + model_dir
    os.chdir(model_dir_location)

    fun_name = model_name + '_impl_dae_fun'
    impl_dae_fun.generate(fun_name, casadi_opts)

    fun_name = model_name + '_impl_dae_fun_jac_x_xdot_z'
    impl_dae_fun_jac_x_xdot_z.generate(fun_name, casadi_opts)
    
    fun_name = model_name + '_impl_dae_jac_x_xdot_u_z'
    impl_dae_jac_x_xdot_u_z.generate(fun_name, casadi_opts)

    fun_name = model_name + '_impl_dae_fun_jac_x_xdot_u_z'
    impl_dae_fun_jac_x_xdot_u_z.generate(fun_name, casadi_opts)

    if generate_hess:
        fun_name = model_name + '_impl_dae_hess'
        impl_dae_hess.generate(fun_name, casadi_opts)

    os.chdir('../..')
