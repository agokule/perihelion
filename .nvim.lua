vim.o.autochdir = false

vim.lsp.enable('clangd')

vim.keymap.set('n', '<leader>rh', function() require('taskrunner').run({ 'just', 'build-web' }) end)
vim.keymap.set('n', '<leader>rH', function() require('taskrunner').run({ 'just', 'build-web', 'release' }) end)

vim.keymap.set('n', '<leader>rb', function() require('taskrunner').run({ 'just', 'build' }) end)
vim.keymap.set('n', '<leader>rB', function() require('taskrunner').run({ 'just', 'build', 'release' }) end)

vim.keymap.set('n', '<leader>rw', function() require('taskrunner').run({ 'just', 'web-open' }) end)
vim.keymap.set('n', '<leader>rp', function()
    vim.cmd('botright 20sp')
    vim.cmd.terminal('./build/bin/Perihelion')
end)

vim.keymap.set('n', '<leader>fm', function() vim.cmd.edit('./src/main.cpp') end)
vim.keymap.set('n', '<leader>fp', function() vim.cmd.edit('./src/Preset.hpp') end)
vim.keymap.set('n', '<leader>fs', function() vim.cmd.edit('./src/SimulationScreen.hpp') end)
vim.keymap.set('n', '<leader>fo', function() vim.cmd.edit('./src/Object.hpp') end)
vim.keymap.set('n', '<leader>fO', function() vim.cmd.edit('./src/ui/ObjectEditor.hpp') end)

