#if !defined(FOR_EACH_RESOURCE_TYPE)
#  error "Define FOR_EACH_RESOURCE_TYPE before including ForEachResourceType.inl"
#endif // !defined(FOR_EACH_RESOURCE_TYPE)

FOR_EACH_RESOURCE_TYPE(Material)
FOR_EACH_RESOURCE_TYPE(Mesh)
FOR_EACH_RESOURCE_TYPE(ShaderModule)
FOR_EACH_RESOURCE_TYPE(Texture)
