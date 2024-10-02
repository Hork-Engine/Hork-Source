echo off

MaterialCompiler -s Data/default/materials/mg/default.mg -o Data/default/materials/compiled/default.mat
MaterialCompiler -s Data/default/materials/mg/default_orm.mg -o Data/default/materials/compiled/default_orm.mat
MaterialCompiler -s Data/default/materials/mg/default_sslr.mg -o Data/default/materials/compiled/default_sslr.mat
MaterialCompiler -s Data/default/materials/mg/emission.mg -o Data/default/materials/compiled/emission.mat
MaterialCompiler -s Data/default/materials/mg/metal_roughness.mg -o Data/default/materials/compiled/metal_roughness.mat
MaterialCompiler -s Data/default/materials/mg/mirror.mg -o Data/default/materials/compiled/mirror.mat
MaterialCompiler -s Data/default/materials/mg/pbr_base_color.mg -o Data/default/materials/compiled/pbr_base_color.mat
MaterialCompiler -s Data/default/materials/mg/skybox.mg -o Data/default/materials/compiled/skybox.mat
MaterialCompiler -s Data/default/materials/mg/unlit.mg -o Data/default/materials/compiled/unlit.mat
MaterialCompiler -s Data/default/materials/mg/unlit_clamped.mg -o Data/default/materials/compiled/unlit_clamped.mat

pause
