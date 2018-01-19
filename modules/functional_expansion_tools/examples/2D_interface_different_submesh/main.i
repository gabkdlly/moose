# Derived from the example '2D_interface' with the following differences:
#
#   1) The number of y divisions in the sub app is not the same as the master app
#   2) The subapp mesh is skewed in y
#   3) The FE order for the flux term was increased to 7
[Mesh]
  type = GeneratedMesh
  dim = 2
  xmin = 0.0
  xmax = 0.4
  nx = 6
  ymin = 0.0
  ymax = 10.0
  ny = 20
[]

[Variables]
  [./m]
  [../]
[]

[Kernels]
  [./diff_m]
    type = HeatConduction
    variable = m
  [../]
  [./time_diff_m]
    type = HeatConductionTimeDerivative
    variable = m
  [../]
  [./source_m]
    type = BodyForce
    variable = m
    value = 100
  [../]
[]

[Materials]
  [./Impervium]
    type = GenericConstantMaterial
    prop_names =  'thermal_conductivity specific_heat density'
    prop_values = '0.00001              50.0          100.0' # W/(cm K), J/(g K), g/cm^3
  [../]
[]

[ICs]
  [./start_m]
    type = ConstantIC
    value = 2
    variable = m
  [../]
[]

[BCs]
  [./interface_value]
    type = FEValueBC
    variable = m
    boundary = right
    function = FE_Basis_Value_Main
  [../]
  [./interface_flux]
    type = FEFluxBC
    boundary = right
    variable = m
    function = FE_Basis_Flux_Main
  [../]
[]

[Functions]
  [./FE_Basis_Value_Main]
    type = FunctionSeries
    series_type = Cartesian
    orders = '4'
    physical_bounds = '0.0 10'
    y = Legendre
  [../]
  [./FE_Basis_Flux_Main]
    type = FunctionSeries
    series_type = Cartesian
    orders = '7'
    physical_bounds = '0.0 10'
    y = Legendre
  [../]
[]

[UserObjects]
  [./FE_Flux_UserObject_Main]
    type = FEBoundaryFluxUserObject
    function = FE_Basis_Flux_Main
    variable = m
    boundary = right
    diffusivity = thermal_conductivity
  [../]
[]

[Postprocessors]
  [./average_interface_value]
    type = SideAverageValue
    variable = m
    boundary = right
  [../]
  [./total_flux]
    type = SideFluxIntegral
    variable = m
    boundary = right
    diffusivity = thermal_conductivity
  [../]
  [./picard_iterations]
    type = NumPicardIterations
    execute_on = 'initial timestep_end'
  [../]
[]

[Executioner]
  type = Transient
  num_steps = 10
  dt = 1.0
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
  picard_max_its = 30
  nl_rel_tol = 1e-8
  nl_abs_tol = 1e-9
  picard_rel_tol = 1e-8
  picard_abs_tol = 1e-9
[]

[Outputs]
  exodus = true
[]

[MultiApps]
  [./FETransferApp]
    type = TransientMultiApp
    input_files = sub.i
    sub_cycling = true
  [../]
[]

[Transfers]
  [./FluxToSub]
    type = MultiAppFETransfer
    direction = to_multiapp
    multi_app = FETransferApp
    this_app_object_name = FE_Flux_UserObject_Main
    multi_app_object_name = FE_Basis_Flux_Sub
  [../]
  [./ValueToMe]
    type = MultiAppFETransfer
    direction = from_multiapp
    multi_app = FETransferApp
    this_app_object_name = FE_Basis_Value_Main
    multi_app_object_name = FE_Value_UserObject_Sub
  [../]
  [./FluxToMe]
    type = MultiAppFETransfer
    direction = from_multiapp
    multi_app = FETransferApp
    this_app_object_name = FE_Basis_Flux_Main
    multi_app_object_name = FE_Flux_UserObject_Sub
  [../]
[]
