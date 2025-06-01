AnimFSM = {
    current = "Idle",
    
    Update = function(self, dt)
        -- self.current = "Contents/Fbx/Capoeira.fbx"

        self.current = "Contents/Human/FastRun"
        self.current = "Contents/Human/NarutoRun"

        return {
            anim = self.current,
            blend = 5.0
        }
    end
}

return AnimFSM