
AnimFSM = {
    current = "Idle",
    
    Update = function(self, dt)
        -- self.current = "Contents/Fbx/Capoeira.fbx"

        self.current = "Contents/Walking/mixamo.com"

        return {
            anim = self.current,
            blend = 5.0
        }
    end,

    ChangeMode = function(self)
        self.current = "Contents/Armature|GunPlay"
    end,
}

return AnimFSM
