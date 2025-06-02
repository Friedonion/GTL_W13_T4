AnimFSM = {
    current = "Idle",
    
    Update = function(self, dt)
        -- self.current = "Contents/Fbx/Capoeira.fbx"

        self.current = "Contents/Human/FastRun"

        return {
            anim = self.current,
            blend = 5.0
        }
    end,

    ChangeMode = function()
        self.current = "Contents/Armature|GunPlay"
    end,
}

return AnimFSM
