AnimFSM = {
    current = "Idle",
    
    Update = function(self, dt)
        self.current = "Contents/Armature|GunPlay"

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
